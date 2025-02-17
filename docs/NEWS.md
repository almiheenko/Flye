Flye 2.5 release (25 Jul 2019)
==============================
* Better ONT polishing for the latest basecallers (Guppy/flipflop)
* Improved consensus quality of repetitive regions
* More contiguous assemblies of real metagenomes
* Improvements for human genome assemblies
* Various bugfixes and performance optimizations

Flye 2.4.2 release (06 Apr 2019)
================================
* Improvements in k-mer selection and tip clipping for metagenome assemblies
* Better memory managment during consensus/polishing
* Some bugfixes

Flye 2.4.1 release (05 Mar 2019)
================================
* Speed and stability improvements for large datasets
* New option `--polish-target` to run Flye polisher on the target sequence


Flye 2.4 release (14 Jan 2019)
==============================
* Metagenome assembly support fully integrated (`--meta` option)
* New Trestle module for resolving simple unbridged repeats
* New `--plasmids` option that recovers short unassembled plasmids

Flye 2.3.7 (14 Nov 2018)
=======================
* Improvements in repeat edges detection
* More precise read mapping - more contiguous assemblies for some datasets
* Memory and performance optimizations for high-coverage datasets
* More accurate repeat graphs for complex datasets


Flye 2.3.6 (24 Sep 2018)
========================
* Memory consumption for large genome assemblies reduced by ~30%
* It could be reduced even further by using the new option --asm-coverage,
which specifies a subset of reads for initial contig assembly
* Better repeat graph representation for complex genomes
* Various bugfixes and stability improvements

Flye 2.3.5 (7 August 2018)
==========================
* New solid kmer alignment implementation with improved specificity
* Better corrected reads support
* Minimum overlap is now selected within a wider range for better support of datasets with shorter read length
* Assembly of large (human size) genomes is now faster
* Various bugfixes and stability improvements

Flye 2.3.4 (19 May 2018)
========================
* A fix for assemblies with low reads count
* Polishing of large genomes is now 2-3x faster and requires less memory

Flye 2.3.3 (27 Mar 2018)
========================
* Automatic selection of minimum overlap parameter based on read length
* Improvements in large genome assemblies
* Minimap2 updated
* Other bugfixes

Flye 2.3.2 (19 Feb 2018)
========================
* Better contiguity for larger genome assemblies
* Improvements in corrected reads mode
* Various bugfixes

Flye 2.3.1 (13 Jan 2018)
========================
* Minor release with a few bugs fixed

Flye 2.3 (04 Jan 2018)
======================

* ABruijn 2.x branch has been renamed to Flye, highlighting many substantial algorithmic changes
* Stable version of the repeat analysis module
* New command-line syntax (fallback mode with the old syntax is available)
* New --subassemblies mode for generating consensus of multiple assemblies
* Improved performance and reduced memory footprint (now scales to human genome)
* Corrected reads are now supported
* Extra output with information about the contigs (coverage, multiplicity, graph paths etc.)
* Gzipped Fasta/q support
* Multiple read files support
* Various bugfixes

ABruijn 2.2b (07 Oct 2017)
==========================

* Various bug fixes and improvements
* More accurate assembly graph construction
* Support for very long (100kb+) reads
* Fastq input

ABruijn 2.1b (11 Sep 2017)
==========================

* Various bug fixes and performance improvements

ABruijn 2.0b (25 Jul 2017)
==========================

* A new repeat graph analysis module for more complete and accurate assembly
* ABruijn now outputs a graph representation of the final assembly
* Significant improvements in performance and reduced memory footprint
* Various bugfixes
