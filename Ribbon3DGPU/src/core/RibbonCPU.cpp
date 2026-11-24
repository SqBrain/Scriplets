#include "RibbonCPU.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

#include "../kernel/RibbonKernel.h"

namespace rb {

void renderCPU(const Packet& p, const float* src, float* dst, const float* tex, int threads) {
    const RK_Params* P = &p.params;
    static const float kNoTexture[4] = {0, 0, 0, 0};
    const float* texture = tex ? tex : kNoTexture;
    if (threads <= 0) threads = int(std::max(1u, std::thread::hardware_concurrency()));
    threads = std::min(threads, std::max(1, P->height));

    std::atomic<int> nextRow{0};
    auto work = [&]() {
        for (int y = nextRow++; y < P->height; y = nextRow++)
            for (int x = 0; x < P->width; x++)
                rk_render_pixel(x, y, P, p.tris.data(), p.verts.data(), p.tileStart.data(), p.tileTris.data(),
                                p.lights.data(), texture, src, dst);
    };
    std::vector<std::thread> pool;
    for (int i = 1; i < threads; i++) pool.emplace_back(work);
    work();
    for (auto& t : pool) t.join();
}

}  // namespace rb
