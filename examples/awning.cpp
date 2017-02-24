/* Copyright (c) 2017 Big Ladder Software and Chip Barnaby. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

// Standard
#include <iostream>

// Penumbra
#include <penumbra/penumbra.h>

int main(void)
{
    Pumbra::Polygon wallVerts =
    {
      0.f, 0.f, 0.f,
      1.f, 0.f, 0.f,
      1.f, 0.f, 1.f,
      0.f, 0.f, 1.f
    };

    Pumbra::Polygon windowVerts =
    {
      0.25f, 0.f, 0.25f,
      0.75f, 0.f, 0.25f,
      0.75f, 0.f, 0.5f,
      0.25f, 0.f, 0.5f
    };

    Pumbra::Polygon awningVerts =
    {
      0.25f, 0.f, 0.5f,
      0.75f, 0.f, 0.5f,
      0.75f, -0.5f, 0.5f,
      0.25f, -0.5f, 0.5f
    };

    Pumbra::Surface wall(wallVerts);
    wall.addHole(windowVerts);

    Pumbra::Surface window(windowVerts);
    Pumbra::Surface awning(awningVerts);

    Pumbra::Penumbra pumbra;

    unsigned wallId = pumbra.addSurface(wall);
    unsigned windowId = pumbra.addSurface(window);
    unsigned awningId = pumbra.addSurface(awning);

    pumbra.setModel();
    pumbra.setSunPosition(2.00f,0.4f);
    pumbra.setSunPosition(3.14f, 0.0f);
    pumbra.renderScene(wallId);
    float wallPSSF = pumbra.calculatePSSF(wallId);

    std::cout << "Wall PSSF: " << wallPSSF << std::endl;

    pumbra.renderScene(windowId);
    float windowPSSF = pumbra.calculatePSSF(windowId);

    std::cout << "Window PSSF: " << windowPSSF << std::endl;

    return 0;
}
