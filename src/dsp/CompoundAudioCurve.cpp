/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2010 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "CompoundAudioCurve.h"

#include "MovingMedian.h"

#include <iostream>

namespace RubberBand
{


CompoundAudioCurve::CompoundAudioCurve(Parameters parameters) :
    AudioCurveCalculator(parameters),
    m_percussive(parameters),
    m_hf(parameters),
    m_hfFilter(new MovingMedian<double>(19, 85)),
    m_hfDerivFilter(new MovingMedian<double>(19, 90)),
    m_lastHf(0.0),
    m_lastResult(0.0),
    m_risingCount(0)
{
    std::cerr << "CompoundAudioCurve::CompoundAudioCurve: window " 
              << parameters.windowSize << ", rate " << parameters.sampleRate
              << std::endl;
}

CompoundAudioCurve::~CompoundAudioCurve()
{
    delete m_hfFilter;
    delete m_hfDerivFilter;
}

void
CompoundAudioCurve::setType(Type type)
{
    std::cerr << "CompoundAudioCurve::setType to " << type << std::endl;
    m_type = type;
}

void
CompoundAudioCurve::reset()
{
    m_percussive.reset();
    m_hf.reset();
    m_hfFilter->reset();
    m_hfDerivFilter->reset();
    m_lastHf = 0.0;
    m_lastResult = 0.0;
}

void
CompoundAudioCurve::setWindowSize(int newSize)
{
    m_percussive.setWindowSize(newSize);
    m_hf.setWindowSize(newSize);
    m_windowSize = newSize;
    m_lastHf = 0.0;
    m_lastResult = 0.0;
}

float
CompoundAudioCurve::processFloat(const float *R__ mag, int increment)
{
    float percussive = 0.f;
    float hf = 0.f;
    switch (m_type) {
    case PercussiveDetector:
        percussive = m_percussive.processFloat(mag, increment);
        break;
    case CompoundDetector:
        percussive = m_percussive.processFloat(mag, increment);
        hf = m_hf.processFloat(mag, increment);
        break;
    case SoftDetector:
        hf = m_hf.processFloat(mag, increment);
        break;
    }
    return processFiltering(percussive, hf);
}

double
CompoundAudioCurve::processDouble(const double *R__ mag, int increment)
{
    double percussive = 0.0;
    double hf = 0.0;
    switch (m_type) {
    case PercussiveDetector:
        percussive = m_percussive.processDouble(mag, increment);
        break;
    case CompoundDetector:
        percussive = m_percussive.processDouble(mag, increment);
        hf = m_hf.processDouble(mag, increment);
        break;
    case SoftDetector:
        hf = m_hf.processDouble(mag, increment);
        break;
    }
    return processFiltering(percussive, hf);
}

double
CompoundAudioCurve::processFiltering(double percussive, double hf)
{
    if (m_type == PercussiveDetector) {
        if (percussive > 0.35) {
            return 1.0;
        } else {
            return 0.0;
        }
    }

    double rv = 0.f;
    
    double hfDeriv = hf - m_lastHf;

    m_hfFilter->push(hf);
    m_hfDerivFilter->push(hfDeriv);

    double hfFiltered = m_hfFilter->get();
    double hfDerivFiltered = m_hfDerivFilter->get();

    m_lastHf = hf;

    double result = 0.f;
    
    double hfExcess = hf - hfFiltered;

    if (hfExcess > 0.0) {
        result = hfDeriv - hfDerivFiltered;
    }

    if (m_type != SoftDetector && percussive > 0.35 && hfExcess > 0.0) {
        rv = 1.0;
        m_risingCount = 0;
    } else {
        if (result < m_lastResult) {
            if (m_risingCount > 3 && m_lastResult > 0) rv = 0.5;
            m_risingCount = 0;
        } else {
            m_risingCount ++;
        }
    }

    m_lastResult = result;

    return rv;
}


}
