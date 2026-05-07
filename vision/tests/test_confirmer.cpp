// firmware/vision/tests/test_confirmer.cpp
#include "vision/confirmer.h"
#include <cassert>
#include <cstdio>
#include <memory>

using namespace hs::vision;

static std::shared_ptr<Track> make_track(int id, ClassId target_cls, float conf,
                                          int age, int n_match, int n_total,
                                          int x_start, int x_end, int n_hist) {
    auto t = std::make_shared<Track>();
    t->id = id;
    t->cls = target_cls;
    t->conf = conf;
    t->age_frames = age;
    for (int i = 0; i < n_total; ++i) {
        ClassId c = (i < n_match) ? target_cls : ClassId::Bee;
        t->class_history.push_back(c);
        t->conf_history.push_back(c == target_cls ? conf : 0.5f);
    }
    for (int i = 0; i < n_hist; ++i) {
        int x = x_start + (x_end - x_start) * i / std::max(1, n_hist - 1);
        t->bbox_history.push_back(cv::Rect(x, 200, 40, 40));
    }
    if (!t->bbox_history.empty()) t->bbox = t->bbox_history.back();
    return t;
}

int main() {
    Confirmer::Params p; p.cooldown_us = 0;  // no cooldown for first eval
    Confirmer c(p);

    // Track that should fire: 4/5 velutina, conf 0.85, age 30, slow velocity.
    auto fire_t = make_track(1, ClassId::Velutina, 0.85f, 30, 4, 5,
                             100, 110 /*delta=10 px over 30 frames @ 30 fps -> 10 px/s*/, 30);
    auto dec = c.evaluate({fire_t}, 30.0, 1'000'000);
    assert(dec.fire);
    assert(dec.track_id == 1);

    // Cooldown blocks immediate refire.
    Confirmer::Params p2; p2.cooldown_us = 1'000'000;
    Confirmer c2(p2);
    auto t2 = make_track(2, ClassId::Velutina, 0.85f, 30, 4, 5, 100, 110, 30);
    auto t3 = make_track(3, ClassId::Velutina, 0.85f, 30, 4, 5, 100, 110, 30);
    assert(c2.evaluate({t2}, 30.0, 1'000'000).fire);
    assert(!c2.evaluate({t3}, 30.0, 1'500'000).fire);   // 0.5 s later
    assert( c2.evaluate({t3}, 30.0, 2'500'000).fire);   // 1.5 s later

    // Each gate fails individually.
    auto bad_age   = make_track(4, ClassId::Velutina, 0.85f,  3, 4, 5, 100, 110, 30);
    auto bad_class = make_track(5, ClassId::Velutina, 0.85f, 30, 1, 5, 100, 110, 30);
    auto bad_conf  = make_track(6, ClassId::Velutina, 0.40f, 30, 4, 5, 100, 110, 30);
    auto bad_vel   = make_track(7, ClassId::Velutina, 0.85f, 30, 4, 5, 100, 9000, 30); // very fast
    Confirmer cc; // fresh confirmer per assertion
    assert(!Confirmer().evaluate({bad_age},   30.0, 1'000'000).fire);
    assert(!Confirmer().evaluate({bad_class}, 30.0, 1'000'000).fire);
    assert(!Confirmer().evaluate({bad_conf},  30.0, 1'000'000).fire);
    assert(!Confirmer().evaluate({bad_vel},   30.0, 1'000'000).fire);

    // Strike zone exclusion.
    Confirmer::Params pz;
    pz.strike_zone = { {0,0}, {50,0}, {50,50}, {0,50} };  // small zone
    Confirmer cz(pz);
    auto out_of_zone = make_track(8, ClassId::Velutina, 0.85f, 30, 4, 5, 100, 110, 30);
    assert(!cz.evaluate({out_of_zone}, 30.0, 1'000'000).fire);

    std::printf("ok\n");
    return 0;
}
