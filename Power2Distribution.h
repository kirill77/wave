#pragma once

#include "MyMisc.h"
#include <algorithm>

// generates power-of-two distribution around fMaxPDFLocation. it works like this: divide distance between 0
// and fMaxPDFLocation to (1 << MAX_LEFT_EXPONENT) intervals and take result as smallest interval. those smallest
// intervals occur only immediately to the left and right of fMaxPDFLocation. as we go farther from
// fMaxPDFLocation, interval sizes increase 2x. each interval independently of size receives the same number
// of samples, so the intervals near fMaxPDFLocation are being sampled more densely
struct Power2Distribution
{
    static const NvU32 MAX_LEFT_EXPONENT = 5;
    static const NvU32 MAX_RIGHT_EXPONENT = 8;

#if ASSERT_ONLY_CODE
    // the integral from this function must be equal to 30 / 2 + 120 / 2 = 75
    static double dbgF(double x)
    {
        if (x < 1) return 0;
        if (x < 31) return (x - 1) / (31 - 1);
        if (x < 151) return (151 - x) / (151 - 31);
        return 0;
    }
    static bool dbgDoesTestPass()
    {
        // use generate() function to compute integral of function dbgF
        double fMin[2] = { 1e38, 1e38 };
        double fMax[2] = { -1e38, -1e38 };
        double fPrevValue = 0, fPrevPrevValue = 0;
        double fSum[2] = { 0, 0 };
        double fWeightsSum[2] = { 0, 0 };
        const NvU32 nSamples = 1024 * 16;
        for (NvU32 u = 0; u < nSamples; ++u)
        {
            const double f01Number = u / (double)(nSamples - 1);
            // uniform sampling
            {
                double fWeight = 1;
                double fValue = f01Number * 152;

                fMin[0] = std::min(fMin[0], fValue);
                fMax[0] = std::max(fMax[0], fValue);
                fSum[0] += dbgF(fValue) * fWeight;
                fWeightsSum[0] += fWeight;
            }
            // fancy sampling
            {
                double fWeight;
                double fValue = generate(f01Number, 31, fWeight);

                bool isOnTheRight = fValue > 31;

                if (fPrevPrevValue != 0)
                {
                    // as we go from left to right, distance between values decreases when we reach the middle and start increasing afterwards
                    if (isOnTheRight)
                    {
                        nvAssert(fValue - fPrevValue >= (fPrevValue - fPrevPrevValue) / 1.1);
                        nvAssert(fValue - fPrevValue <= (fPrevValue - fPrevPrevValue) * 2.1);
                    }
                    else
                    {
                        nvAssert(fValue - fPrevValue <= (fPrevValue - fPrevPrevValue) * 1.1);
                        nvAssert(fValue - fPrevValue >= (fPrevValue - fPrevPrevValue) / 2.1);
                    }
                }
                fPrevPrevValue = fPrevValue;
                fPrevValue = fValue;

                fMin[1] = std::min(fMin[1], fValue);
                fMax[1] = std::max(fMax[1], fValue);
                fSum[1] += dbgF(fValue) * fWeight;
                fWeightsSum[1] += fWeight;
            }
        }
        double fFinalValue[2] = { fSum[0] / fWeightsSum[0] * (fMax[0] - fMin[0]), fSum[1] / fWeightsSum[1] * (fMax[1] - fMin[1]) };
        // fancy sampling comes out less presize than uniform one, but i hope it will work better for action-related functions
        return abs(fFinalValue[0] - 75) < 0.01 && abs(fFinalValue[1] - 75) < 0.03;
    }
#endif

    static double generate(double fIn01Number, double fMaxPDFLocation, double& fOutWeight)
    {
        double fSmallestIntervalSize = fMaxPDFLocation / (1 << MAX_LEFT_EXPONENT);
        double fSplit = MAX_LEFT_EXPONENT / (double)(MAX_LEFT_EXPONENT + MAX_RIGHT_EXPONENT);
        // generate sample on the left or right?
        if (fIn01Number < fSplit)
        {
            fIn01Number = 1 - fIn01Number / fSplit; // we want 0 to correspond to max exponent (left boundary)
            double fExponent = fIn01Number * MAX_LEFT_EXPONENT;
            NvU32 uExponent = std::min((NvU32)fExponent, MAX_LEFT_EXPONENT - 1);
            fIn01Number = fExponent - uExponent;
            //double fRightBoundary = 1 - fSmallestIntervalSize * ((1 << uExponent) - 1);
            //return fRightBoundary - fIntervalSize * fIn01Number;
            fOutWeight = (1 << uExponent);
            return fMaxPDFLocation - fSmallestIntervalSize * ((1 << uExponent) * (fIn01Number + 1) - 1);
        }
        fIn01Number = (fIn01Number - fSplit) / (1 - fSplit);
        double fExponent = fIn01Number * MAX_RIGHT_EXPONENT;
        NvU32 uExponent = std::min((NvU32)fExponent, MAX_RIGHT_EXPONENT - 1);
        fIn01Number = fExponent - uExponent;
        //double fLeftBoundary = 1 + fSmallestIntervalSize * ((1 << uExponent) - 1);
        //return fLeftBoundary + fIntervalSize * fIn01Number;
        fOutWeight = (1 << uExponent);
        return fMaxPDFLocation + fSmallestIntervalSize * ((1 << uExponent) * (fIn01Number + 1) - 1);
    }
};