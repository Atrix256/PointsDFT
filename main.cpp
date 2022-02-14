#include <array>
#include <vector>
#include <direct.h>
#include <random>


// TODO: unify these structures as "average tests" or something

struct
{
    size_t testCount = 10000;
    size_t sampleCount = 10;
    int maxHz = 20;

} g_BlueNoiseTest;

struct
{
    size_t testCount = 10000;
    size_t sampleCount = 10;
    int maxHz = 20;

} g_WhiteNoiseTest;

static const float c_pi = 3.14159265359f;
static const float c_twoPi = c_pi * 2.0f;

struct DFTRow
{
    int hz;
    float re;
    float im;
    float mag;
    float phase;
};

float Lerp(float A, float B, float t)
{
    return A * (1.0f - t) + B * t;
}

void WriteDFTRows(const std::vector<DFTRow>& rows, const char* fileName)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "wb");
    fprintf(file, "\"hz\",\"re\",\"im\",\"mag\",\"phase\"\n");

    for (const DFTRow& row : rows)
        fprintf(file, "\"%i\",\"%f\",\"%f\",\"%f\",\"%f\"\n", row.hz, row.re, row.im, row.mag, row.phase);

    fclose(file);
}

void WriteDFTRowsMag(const std::vector<DFTRow>& rows, const char* fileName)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "wb");
    fprintf(file, "\"hz\",\"mag\"\n");

    for (const DFTRow& row : rows)
        fprintf(file, "\"%i\",\"%f\"\n", row.hz, row.mag);

    fclose(file);
}

std::vector<DFTRow> DFTPoints(const std::vector<float>& points, int minHz, int maxHz)
{
    std::vector<DFTRow> ret;
    float div = float(points.size());
    for (int hz = minHz; hz <= maxHz; ++hz)
    {
        float re = 0.0f;
        float im = 0.0f;

        for (float f : points)
        {
            re += cosf(c_twoPi * float(hz) * f) / div;
            im += sinf(c_twoPi * float(hz) * f) / div;
        }

        DFTRow row;
        row.hz = hz;
        row.re = re;
        row.im = im;
        row.mag = sqrtf(re * re + im * im);
        row.phase = atan2f(im, re);
        ret.push_back(row);
    }

    return ret;
}

// Blue noise is <true, true>
template <bool CandidateScoreIsMinDistance, bool BestCandidateIsMaxScore>
std::vector<float> MBC(size_t count, int candidateMultiplier)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<float> ret;
    ret.reserve(count);

    for (size_t index = 0; index < count; ++index)
    {
        size_t candidateCount = (index * candidateMultiplier) + 1;
        float bestCandidate = 0.0f;
        float bestCandidateScore = 0.0f;

        for (size_t candidateIndex = 0; candidateIndex < candidateCount; ++candidateIndex)
        {
            float candidate = dist(rng);
            float candidateScore = 0.0f;

            int pointIndex = 0;
            for (float f : ret)
            {
                float dist = std::abs(f - candidate);
                if (CandidateScoreIsMinDistance)
                {
                    dist = std::min(dist, 1.0f - dist);
                    if (pointIndex == 0 || dist < candidateScore)
                        candidateScore = dist;
                }
                else
                {
                    dist = std::max(dist, 1.0f - dist);
                    if (pointIndex == 0 || dist > candidateScore)
                        candidateScore = dist;
                }
                pointIndex++;
            }

            if (BestCandidateIsMaxScore)
            {
                if (candidateIndex == 0 || candidateScore > bestCandidateScore)
                {
                    bestCandidate = candidate;
                    bestCandidateScore = candidateScore;
                }
            }
            else
            {
                if (candidateIndex == 0 || candidateScore < bestCandidateScore)
                {
                    bestCandidate = candidate;
                    bestCandidateScore = candidateScore;
                }
            }
        }

        ret.push_back(bestCandidate);
    }

    return ret;
}

void WritePoints(const std::vector<float>& _points, const char* fileName)
{
    std::vector<float> points = _points;
    std::sort(points.begin(), points.end());

    FILE* file = nullptr;
    fopen_s(&file, fileName, "wb");

    for (float f : points)
        fprintf(file, "\"%f\"\n", f);

    fclose(file);

}

template <bool CandidateScoreIsMinDistance, bool BestCandidateIsMaxScore>
void MBCTest(int candidateMultiplier, const char* fileNameBase)
{
    char fileName[1024];
    printf(__FUNCTION__"(%i, \"%s\")\n", candidateMultiplier, fileNameBase);
    std::vector<DFTRow> DFTAvg;

    for (size_t testIndex = 0; testIndex < g_BlueNoiseTest.testCount; ++testIndex)
    {
        std::vector<float> points = MBC<CandidateScoreIsMinDistance, BestCandidateIsMaxScore>(g_BlueNoiseTest.sampleCount, candidateMultiplier);
        std::vector<DFTRow> DFT = DFTPoints(points, -g_BlueNoiseTest.maxHz, g_BlueNoiseTest.maxHz);
        if (testIndex == 0)
        {
            sprintf_s(fileName, "out/%s_%i.points.csv", fileNameBase, candidateMultiplier);
            WritePoints(points, fileName);
            DFTAvg = DFT;
            continue;
        }

        for (size_t i = 0; i < DFTAvg.size(); ++i)
            DFTAvg[i].mag = Lerp(DFTAvg[i].mag, DFT[i].mag, 1.0f / float(testIndex+1));
    }
    
    sprintf_s(fileName, "out/%s_%i.avg.csv", fileNameBase, candidateMultiplier);
    WriteDFTRowsMag(DFTAvg, fileName);
}

void WhiteNoiseTest()
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<DFTRow> DFTAvg;

    for (size_t testIndex = 0; testIndex < g_WhiteNoiseTest.testCount; ++testIndex)
    {
        std::vector<float> whiteNoise(g_WhiteNoiseTest.sampleCount);
        for (float& f : whiteNoise)
            f = dist(rng);
        std::vector<DFTRow> DFT = DFTPoints(whiteNoise, -g_WhiteNoiseTest.maxHz, g_WhiteNoiseTest.maxHz);
        if (testIndex == 0)
        {
            DFTAvg = DFT;
            continue;
        }

        for (size_t i = 0; i < DFTAvg.size(); ++i)
            DFTAvg[i].mag = Lerp(DFTAvg[i].mag, DFT[i].mag, 1.0f / float(testIndex + 1));
    }

    WriteDFTRowsMag(DFTAvg, "out/whitenoiseavg.csv");
}

int main(int argc, char** argv)
{
    _mkdir("out");

    WriteDFTRows(DFTPoints({ 0.0f, 1.0f / 4.0f, 3.0f / 4.0f }, -10, 10), "out/1.csv");
    WriteDFTRows(DFTPoints({ 0.822120f, 0.547486f, 0.234054f, 0.428338f, 0.78547f, 0.734857f, 0.646972f, 0.941578f, 0.492718f, 0.327577f }, -25, 25), "out/2.csv");

    MBCTest<true, true>(1, "blue");
    MBCTest<true, true>(2, "blue");
    MBCTest<true, true>(5, "blue");

    MBCTest<true, false>(1, "red");
    MBCTest<true, false>(2, "red");
    MBCTest<true, false>(5, "red");

    // This also makes blue noise
    //MBCTest<false, false>(1, "MBC00");

    // This also makes red noise
    //MBCTest<false, true>(1, "MBC01");

    WhiteNoiseTest();

    return 0;
}

/*

TODO:
* do the same with modified MBC, see if you can make red noise. really want uniform red noise.
 * maybe do all 4 combos. is candidate score the min or max distance from nearest point. is best candidate the min or max of that?
* and golden ratio? sqrt(2)? pi?
* try gradient descent to make red noise and other frequency compositions?
? how many hz should you look at? it repeats for the simple fractions. it must repeat for the blue noise too at some point
! could compare vs an image based DFT to see how accurate this is.
! Do this in 2D too
! get PCG and vec libraries... move this into internal repo?
? histogram test? to verify it's uniform?
* have python auto generate the graphs so you don't need to keep doing it in open office
*/