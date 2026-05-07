#include "vision/frame_source.h"
#include <cassert>
int main() {
    hs::vision::Frame f;
    assert(!f.valid());
    return 0;
}
