//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#pragma once

#include <unistd.h>

namespace Constants
{
	//global constants
	const int maximumJump = 1500;
	const int maximumOverhang = 500;

	//kmer enumeration
	const int hardMinCoverageRate = 10;
	const int repeatCoverageRate = 10;

	//overlap constants
	const int closeJumpRate = 100;
	const int farJumpRate = 2;
	const int overlapDivergenceRate = 5;

	//chimera detection
	const int maxCoverageDropRate = 5;
	const int chimeraWindow = 100;

	//extension
	const int minReadsInContig = 4;
	const int minExtensions = 2;
	const float startReadsPercent = 0.1;

	//repeat graph
	const int maxSeparation = 1500;
	const int trustedEdgeLength = 10000;
}

struct Parameters
{
	static Parameters& get()
	{
		static Parameters param;
		return param;
	}

	int minimumOverlap;
	size_t kmerSize;
	size_t numThreads;
};
