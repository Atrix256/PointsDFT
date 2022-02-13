#include <array>
#include <vector>
#include <direct.h>

static const float c_pi = 3.14159265359f;
static const float c_twoPi = c_pi * 2.0f;

void DFTPoints(const std::vector<float>& points, int minHz, int maxHz, const char* fileName)
{
    FILE* file = nullptr;
    fopen_s(&file, fileName, "wb");
    fprintf(file, "\"hz\", \"re\", \"im\", \"mag\"\n");

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

        fprintf(file, "\"%i\",\"%f\",\"%f\",\"%f\"\n", hz, re, im, sqrtf(re * re + im * im));
    }

    fclose(file);
}

int main(int argc, char** argv)
{
    _mkdir("out");
    DFTPoints({ 0.0f, 1.0f / 4.0f, 3.0f / 4.0f }, -3, 3, "out/1.csv");
    return 0;
}