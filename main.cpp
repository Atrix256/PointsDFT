#include <array>
#include <vector>
#include <direct.h>
#include <random>

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
            float candidateScore = FLT_MAX;

            for (float f : ret)
            {
                float dist = std::abs(f - candidate);
                dist = std::min(dist, 1.0f - dist);
                candidateScore = std::min(candidateScore, dist);
            }

            if (candidateScore > bestCandidateScore)
            {
                bestCandidate = candidate;
                bestCandidateScore = candidateScore;
            }
        }

        ret.push_back(bestCandidate);
    }

    return ret;
}

void BlueNoiseTest(int candidateMultiplier)
{
    printf("BlueNoiseTest(%i)\n", candidateMultiplier);
    std::vector<DFTRow> DFTAvg;

    for (size_t testIndex = 0; testIndex < g_BlueNoiseTest.testCount; ++testIndex)
    {
        std::vector<float> blueNoise = MBC(g_BlueNoiseTest.sampleCount, candidateMultiplier);
        std::vector<DFTRow> DFT = DFTPoints(blueNoise, -g_BlueNoiseTest.maxHz, g_BlueNoiseTest.maxHz);
        if (testIndex == 0)
        {
            DFTAvg = DFT;
            continue;
        }

        for (size_t i = 0; i < DFTAvg.size(); ++i)
            DFTAvg[i].mag = Lerp(DFTAvg[i].mag, DFT[i].mag, 1.0f / float(testIndex+1));
    }
    
    char fileName[1024];
    sprintf_s(fileName, "out/bluenoiseavg_%i.csv", candidateMultiplier);
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

    BlueNoiseTest(1);
    BlueNoiseTest(5);

    WhiteNoiseTest();

    return 0;
}

/*

TODO:
* do the same with modified MBC, see if you can make red noise. really want uniform red noise.
* and golden ratio? sqrt(2)? pi?
* try gradient descent to make red noise and other frequency compositions?
? how many hz should you look at? it repeats for the simple fractions. it must repeat for the blue noise too at some point
! could compare vs an image based DFT to see how accurate this is.
! Do this in 2D too
! get PCG and vec libraries... move this into internal repo?

*/