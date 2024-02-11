# Loris is Copyright (c) 1999-2010 by Kelly Fitz and Lippold Haken
# <loris@cerlsoundgroup.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# source code for Loris classes
CPP_SRC = AiffData.C	\
		AiffData.h \
		AiffFile.C \
		AiffFile.h \
		Analyzer.C \
		Analyzer.h \
		AssociateBandwidth.C \
		AssociateBandwidth.h \
		BigEndian.C \
		BigEndian.h \
		Breakpoint.C \
		Breakpoint.h \
		BreakpointEnvelope.h \
		BreakpointUtils.C \
		BreakpointUtils.h \
		Channelizer.C \
		Channelizer.h \
		Collator.C \
		Collator.h \
		Dilator.C \
		Dilator.h \
		Distiller.C \
		Distiller.h \
		Envelope.C \
		Envelope.h \
		F0Estimate.C \
		F0Estimate.h \
		LorisExceptions.C \
		LorisExceptions.h \
		Filter.C \
		Filter.h \
		FourierTransform.C \
		FourierTransform.h \
		FrequencyReference.C \
		FrequencyReference.h \
		Fundamental.C \
		Fundamental.h \
		Harmonifier.C \
		Harmonifier.h \
		ImportLemur.C \
		ImportLemur.h \
		KaiserWindow.C \
		KaiserWindow.h \
		LinearEnvelope.C \
		LinearEnvelope.h \
		Marker.C	\
		Marker.h	\
		Morpher.C \
		Morpher.h \
		NoiseGenerator.C \
		NoiseGenerator.h \
		Notifier.C \
		Notifier.h \
		Oscillator.C \
		Oscillator.h \
		Partial.C \
		Partial.h \
		PartialBuilder.C	\
		PartialBuilder.h	\
		PartialList.h \
		PartialPtrs.h \
		PartialUtils.C \
		PartialUtils.h \
		phasefix.C	\
		phasefix.h	\
		ReassignedSpectrum.C \
		ReassignedSpectrum.h \
		Resampler.C \
		Resampler.h \
		SdifFile.h \
		SdifFile.C \
		Sieve.h \
		Sieve.C \
		SpcFile.C \
		SpcFile.h \
		SpectralPeaks.h \
		SpectralPeakSelector.C \
		SpectralPeakSelector.h \
		SpectralSurface.C \
		SpectralSurface.h \
		Synthesizer.C \
		Synthesizer.h \
        fftsg.c


# source code for the procedural (C) interface
PI_SRC = loris.h lorisAnalyzer_pi.C lorisBpEnvelope_pi.C \
 lorisException_pi.C lorisException_pi.h lorisNonObj_pi.C \
 lorisPartialList_pi.C lorisUtilities_pi.C 


# convenience library containing Csound opcodes 
if HAVE_CSOUND
CSOUND_LIB = $(top_builddir)/csound/liblorisops.la
endif


lib_LTLIBRARIES = libloris.la
libloris_la_SOURCES = $(CPP_SRC) $(PI_SRC)
libloris_la_CPPFLAGS = $(INCLUDE_FFTW) $(default_includes)
libloris_la_LIBADD = $(LINK_FFTW) $(CSOUND_LIB)

# the library version for Loris 1.8 is 13:0:0
libloris_la_LDFLAGS = -version-info 13:0:0 $(EXTRA_LD_FLAGS)

# loris.h is generated automatically from loris.h.in
nodist_include_HEADERS = loris.h
EXTRA_DIST = loris.h.in

# installed Loris header files
pkginclude_HEADERS = \
				AiffFile.h		\
				Analyzer.h		\
				BreakpointEnvelope.h	\
				Breakpoint.h	\
				BreakpointUtils.h	\
				Channelizer.h	\
				Collator.h	\
				Dilator.h	\
				Distiller.h	\
				Envelope.h	\
				Exception.h	\
                F0Estimate.h \
				Filter.h	\
				FourierTransform.h	\
				FrequencyReference.h \
				Fundamental.h \
				Harmonifier.h	\
				ImportLemur.h	\
				KaiserWindow.h	\
				LinearEnvelope.h \
				LorisExceptions.h	\
				Marker.h	\
				Morpher.h	\
				NoiseGenerator.h \
				Notifier.h	\
				Oscillator.h	\
				Partial.h	\
				PartialList.h	\
				PartialPtrs.h	\
				PartialUtils.h	\
				ReassignedSpectrum.h	\
				Resampler.h \
				SdifFile.h	\
				Sieve.h	\
				SpcFile.h	\
				SpectralSurface.h	\
				Synthesizer.h

MAINTAINERCLEANFILES = Makefile.in

