#include <array>
#include <vector>
#include <direct.h>

static const float c_pi = 3.14159265359f;
static const float c_twoPi = c_pi * 2.0f;

void DFTPoints(const std::vector<float>& points, int minHz, int maxHz, const char* fileName)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "wb");
    fprintf(file, "\"hz\",\"re\",\"im\",\"mag\",\"phase\"\n");

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

        fprintf(file, "\"%i\",\"%f\",\"%f\",\"%f\",\"%f\"\n", hz, re, im, sqrtf(re * re + im * im), atan2f(im, re));
    }

    fclose(file);
}

int main(int argc, char** argv)
{
    _mkdir("out");

    DFTPoints({ 0.0f, 1.0f / 4.0f, 3.0f / 4.0f }, -10, 10, "out/1.csv");

    DFTPoints({ 0.822120f, 0.547486f, 0.234054f, 0.428338f, 0.78547f, 0.734857f, 0.646972f, 0.941578f, 0.492718f, 0.327577f }, -20, 20, "out/2.csv");

    return 0;
}