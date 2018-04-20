//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <iostream>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <execinfo.h>

#include "../sequence/vertex_index.h"
#include "../sequence/sequence_container.h"
#include "../sequence/overlap.h"
#include "../sequence/consensus_generator.h"
#include "../common/config.h"
#include "chimera.h"
#include "extender.h"
#include "parameters_estimator.h"
#include "../common/logger.h"
#include "../common/utils.h"

#include "../sequence/mm_index.h"

bool parseArgs(int argc, char** argv, std::string& readsFasta, 
			   std::string& outAssembly, std::string& logFile, size_t& genomeSize,
			   int& minKmer, int& maxKmer, bool& debug,
			   size_t& numThreads, int& minOverlap, std::string& configPath,
			   bool& singletonReads)
{
	auto printUsage = [argv]()
	{
		std::cerr << "Usage: " << argv[0]
				  << "\treads_files out_assembly genome_size config_file\n\t"
				  << "[-m min_kmer_cov] \n\t"
				  << "[-x max_kmer_cov] [-l log_file] [-t num_threads] [-d]\n\n"
				  << "positional arguments:\n"
				  << "\treads file\tcomma-separated list of read files\n"
				  << "\tout_assembly\tpath to output file\n"
				  << "\tgenome_size\tgenome size in bytes\n"
				  << "\tconfig_file\tpath to the config file\n"
				  << "\noptional arguments:\n"
				  //<< "\t-k kmer_size\tk-mer size [default = 15] \n"
				  << "\t-m min_kmer_cov\tminimum k-mer coverage "
				  << "[default = auto] \n"
				  << "\t-x max_kmer_cov\tmaximum k-mer coverage "
				  << "[default = auto] \n"
				  << "\t-v min_overlap\tminimum overlap between reads "
				  << "[default = 5000] \n"
				  << "\t-s \t\tadd singleton reads "
				  << "[default = false] \n"
				  << "\t-d \t\tenable debug output "
				  << "[default = false] \n"
				  << "\t-l log_file\toutput log to file "
				  << "[default = not set] \n"
				  << "\t-t num_threads\tnumber of parallel threads "
				  << "[default = 1] \n";
	};

	//kmerSize = -1;
	minKmer = -1;
	maxKmer = -1;
	numThreads = 1;
	debug = false;
	singletonReads = false;
	minOverlap = -1;

	const char optString[] = "m:x:l:t:v:hds";
	int opt = 0;
	while ((opt = getopt(argc, argv, optString)) != -1)
	{
		switch(opt)
		{
		//case 'k':
		//	kmerSize = atoi(optarg);
		//	break;
		case 'm':
			minKmer = atoi(optarg);
			break;
		case 'x':
			maxKmer = atoi(optarg);
			break;
		case 't':
			numThreads = atoi(optarg);
			break;
		case 'v':
			minOverlap = atoi(optarg);
			break;
		case 'l':
			logFile = optarg;
			break;
		case 'd':
			debug = true;
			break;
		case 's':
			singletonReads = true;
			break;
		case 'h':
			printUsage();
			exit(0);
		}
	}
	if (argc - optind != 4)
	{
		printUsage();
		return false;
	}
	readsFasta = *(argv + optind);
	outAssembly = *(argv + optind + 1);
	genomeSize = atoll(*(argv + optind + 2));
	configPath = *(argv + optind + 3);

	return true;
}

bool fileExists(const std::string& path)
{
	std::ifstream fin(path);
	return fin.good();
}

void segfaultHandler(int signal)
{
	void *stackArray[20];
	size_t size = backtrace(stackArray, 10);
	Logger::get().error() << "Segmentation fault! Backtrace:";
	char** backtrace = backtrace_symbols(stackArray, size);
	for (size_t i = 0; i < size; ++i)
	{
		Logger::get().error() << "\t" << backtrace[i];
	}
	exit(1);
}

void exceptionHandler()
{
	static bool triedThrow = false;
	try
	{
        if (!triedThrow)
		{
			triedThrow = true;
			throw;
		}
    }
    catch (const std::exception &e) 
	{
        Logger::get().error() << "Caught unhandled exception: " << e.what();
    }
	catch (...) {}

	void *stackArray[20];
	size_t size = backtrace(stackArray, 10);
	char** backtrace = backtrace_symbols(stackArray, size);
	for (size_t i = 0; i < size; ++i)
	{
		Logger::get().error() << "\t" << backtrace[i];
	}
	exit(1);
}

int getKmerSize(size_t genomeSize)
{
	return (genomeSize < (size_t)Config::get("big_genome_threshold")) ?
			(int)Config::get("kmer_size") : 
			(int)Config::get("kmer_size_big");
}

int chooseMinOverlap(const SequenceContainer& seqReads)
{
	//choose minimum overlap as reads N90
	const float NX_FRAC = 0.90f;
	const int GRADE = 1000;
	int estMinOvlp = round((float)seqReads.computeNxStat(NX_FRAC) / GRADE) * GRADE;
	return std::min(std::max((int)Config::get("low_minimum_overlap"), 
							 estMinOvlp),
					(int)Config::get("high_minimum_overlap"));
}

////////////////////////////////////////////////////////////////

#include <vector>
#include "../sequence/mm_buffer.h"
#include "../sequence/mm_alignment_container.h"
#include "../../lib/minimap2/minimap.h"

std::vector<OverlapRange> findOverlaps(const FastaRecord &fastaRec,
                                       const MinimapIndex &index,
                                       const SequenceContainer &readsContainer)
{
    mm_mapopt_t mopt;
    mm_mapopt_init(&mopt);

    mopt.flag |= MM_F_ALL_CHAINS | MM_F_NO_DIAG | MM_F_NO_DUAL | MM_F_NO_LJOIN;
    mopt.min_chain_score = 100, mopt.pri_ratio = 0.0f, mopt.max_gap = 10000, mopt.max_chain_skip = 25;

    mm_mapopt_update(&mopt, index.get());

    std::vector<OverlapRange> overlaps;
    MinimapBuffer buffer;
    int numOfAlignments;

    std::string cur = fastaRec.sequence.str();
    int curLen = fastaRec.sequence.length();

    mm_reg1_t *pAlignments = mm_map(index.get(), curLen, cur.data(),
                                    &numOfAlignments, buffer.get(), &mopt, 0);

    MinimapAlignmentContainer alignmentContainer(pAlignments, numOfAlignments);

	std::unordered_map<int32_t, OverlapRange> bestOverlapsHash;

    int32_t curId = fastaRec.id.get();
    int32_t curRevCompId = fastaRec.id.rc().get();

	for (int i = 0; i < alignmentContainer.getNumOfAlignments(); ++i)
	{
		int32_t extId = index.getSequenceId(alignmentContainer.getExtIndexId(i));

        if (curId == extId || curRevCompId == extId)
        {
            continue;
        }

        int32_t extLen = index.getSequenceLen(alignmentContainer.getExtIndexId(i));
        int32_t curBegin = alignmentContainer.getCurBegin(i);
        int32_t curEnd = alignmentContainer.getCurEnd(i);
        int32_t extBegin = alignmentContainer.getExtBegin(i);
        int32_t extEnd = alignmentContainer.getExtEnd(i);
        int32_t score = alignmentContainer.getScore(i);

        bool curStrand = alignmentContainer.curStrand(i);
        int32_t extRevComp = FastaRecord::Id(extId).rc().get();

        if (!curStrand)
		{
			extId = extRevComp;
			int32_t newExtBegin = extLen - extEnd - 1;
			int32_t newExtEnd = extLen - extBegin - 1;
			extBegin = newExtBegin;
			extEnd = newExtEnd;
		}

        auto overlap = OverlapRange(curId, curBegin, curEnd, curLen,
                               extId, extBegin, extEnd, extLen,
                               extBegin - curBegin,
                               (extLen - extEnd) - (curLen - curEnd),
                               score);

		auto overlapPair = bestOverlapsHash.find(extId);

		if (overlapPair != bestOverlapsHash.end())
		{
            if (overlapPair->second.score < score)
            {
                overlapPair->second = overlap;
            }
		}
		else
		{
            bestOverlapsHash[extId] = overlap;
		}
	}

    for (auto &overlapPair : bestOverlapsHash)
    {
        overlaps.push_back(overlapPair.second);
    }

    return overlaps;
}

void printOverlapRange(const OverlapRange &overlapRange,
                       const SequenceContainer &readsContainer)
{
    int32_t curId = overlapRange.curId.get();
    int32_t extId = overlapRange.extId.get();

    std::cout << "id" << curId << '\t';
    std::cout << overlapRange.curLen << '\t';
    std::cout << overlapRange.curBegin << '\t';
    std::cout << overlapRange.curEnd << '\t';
    std::cout << "id" << extId << '\t';
    std::cout << overlapRange.extBegin << '\t';
    std::cout << overlapRange.extEnd << '\t';
    std::cout << overlapRange.score << std::endl;

    auto cur = readsContainer.getSeq(overlapRange.curId);
    auto ext = readsContainer.getSeq(overlapRange.extId);

    size_t step = 50;

    for (size_t i = overlapRange.curBegin; i != overlapRange.curBegin + step; ++i)
    {
        std::cout << cur.at(i);
    }
    std::cout << "..........";

    for (size_t i = overlapRange.curEnd - step; i != overlapRange.curEnd; ++i)
    {
        std::cout << cur.at(i);
    }
    std::cout << std::endl;

    for (size_t i = overlapRange.extBegin; i != overlapRange.extBegin + step; ++i)
    {
        std::cout << ext.at(i);
    }
    std::cout << "..........";

    for (size_t i = overlapRange.extEnd - step; i != overlapRange.extEnd; ++i)
    {
        std::cout << ext.at(i);
    }
    std::cout << std::endl;
}

int main(int argc, char** argv)
{
	#ifndef _DEBUG
	signal(SIGSEGV, segfaultHandler);
	std::set_terminate(exceptionHandler);
	#endif

	//int kmerSize = 0;
	int minKmerCov = 0;
	int maxKmerCov = 0;
	size_t genomeSize = 0;
	int minOverlap = -1;
	bool debugging = false;
	bool singletonReads = false;
	size_t numThreads = 1;
	std::string readsFasta;
	std::string outAssembly;
	std::string logFile;
	std::string configPath;

	if (!parseArgs(argc, argv, readsFasta, outAssembly, logFile, genomeSize,
				   minKmerCov, maxKmerCov, debugging, numThreads,
				   minOverlap, configPath, singletonReads)) return 1;

	Logger::get().setDebugging(debugging);
	if (!logFile.empty()) Logger::get().setOutputFile(logFile);
	Logger::get().debug() << "Build date: " << __DATE__ << " " << __TIME__;
	std::ios::sync_with_stdio(false);

	Config::load(configPath);
	Parameters::get().numThreads = numThreads;
	Parameters::get().kmerSize = getKmerSize(genomeSize);
	Logger::get().info() << "Running with k-mer size: " << 
		Parameters::get().kmerSize; 
	//if (kmerSize != -1) Parameters::get().kmerSize = kmerSize; 

	SequenceContainer readsContainer;
	std::vector<std::string> readsList = splitString(readsFasta, ',');
	Logger::get().info() << "Reading sequences";
	try
	{
		for (auto& readsFile : readsList)
		{
			readsContainer.loadFromFile(readsFile);
		}
	}
	catch (SequenceContainer::ParseException& e)
	{
		Logger::get().error() << e.what();
		return 1;
	}

    MinimapIndex minimapIndex(readsContainer);

	/*
	for (auto &hashPair : readsContainer.getIndex())
	{
		if (hashPair.first.get() == 0)
		{
			auto overlaps = findOverlaps(hashPair.second, minimapIndex, readsContainer);
			for (auto &overlap : overlaps)
            {
                printOverlapRange(overlap, readsContainer);
            }
		}
	}
	 */

    //./flye --pacbio-raw E.coli_PacBio_40x-first500.fasta --out-dir out_pacbio --genome-size 5m --threads 1


	//VertexIndex vertexIndex(readsContainer,
	//						(int)Config::get("assemble_kmer_sample"));
	//vertexIndex.outputProgress(true);

	Logger::get().info() << "Reads N50/90: " << readsContainer.computeNxStat(0.50) <<
		" / " << readsContainer.computeNxStat(0.90);

	if (minOverlap == -1) minOverlap = chooseMinOverlap(readsContainer);
	Parameters::get().minimumOverlap = minOverlap;
	Logger::get().info() << "Selected minimum overlap " << minOverlap;

	int64_t sumLength = 0;
	for (auto& seqId : readsContainer.getIndex())
	{
		sumLength += seqId.second.sequence.length();
	}
	int coverage = sumLength / 2 / genomeSize;
	Logger::get().info() << "Expected read coverage: " << coverage;
	if (coverage < 5 || coverage > 1000)
	{
		Logger::get().warning() << "Expected read coverage is " << coverage
			<< ", the assembly is not guaranteed to be optimal in this setting."
			<< " Are you sure that the genome size was entered correctly?";
	}
	if (maxKmerCov == -1)
	{
		maxKmerCov = (int)Config::get("repeat_coverage_rate") * coverage;
	}

	Logger::get().info() << "Generating solid k-mer index";
	size_t hardThreshold = std::min(5, std::max(2, 
			coverage / (int)Config::get("hard_min_coverage_rate")));
	//vertexIndex.countKmers(hardThreshold, genomeSize);

	//ParametersEstimator estimator(readsContainer, vertexIndex, genomeSize);
	//estimator.estimateMinKmerCount(maxKmerCov);
	//if (minKmerCov == -1)
	//{
	//	minKmerCov = estimator.minKmerCount();
	//}

	//vertexIndex.buildIndex(minKmerCov, maxKmerCov);

	OverlapDetector ovlp(readsContainer, minimapIndex,
						 (int)Config::get("maximum_jump"), 
						 Parameters::get().minimumOverlap,
						 (int)Config::get("maximum_overhang"),
						 (int)Config::get("assemble_gap"), false);
	OverlapContainer readOverlaps(ovlp, readsContainer, true);

	Extender extender(readsContainer, readOverlaps, coverage, genomeSize);
	extender.assembleContigs(singletonReads);
	//vertexIndex.clear();

	ConsensusGenerator consGen;
	auto contigsFasta = consGen.generateConsensuses(extender.getContigPaths());
	SequenceContainer::writeFasta(contigsFasta, outAssembly);
	return 0;
}
