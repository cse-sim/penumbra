/* Copyright (c) 2017 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef PENUMBRA_H_
#define PENUMBRA_H_

// Standard
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>

// Penumbra
#include <penumbra/surface.h>
#include <penumbra/logging.h>

namespace Pumbra {

const int PN_SUCCESS = 0;
const int PN_FAILURE = 1;
const int MSG_INFO = 0;
const int MSG_WARN = 1;
const int MSG_ERR = 2;

enum class VendorName { None, NVIDIA, AMD, Intel, VMware };

class PenumbraPrivate;

void penumbraTerminate(); // Call once before exiting calling program to ensure safe cleanup of
                          // OpenGL memory

class Penumbra {
public:
  Penumbra(unsigned size = 512u,
           const std::shared_ptr<Courierr::Courierr> &logger = std::make_shared<PenumbraLogger>());

  ~Penumbra();

public:
  static bool isValidContext();
  unsigned addSurface(const Surface &surface);
  int setModel();
  int clearModel();
  int setSunPosition(const float azm, // in radians, clockwise, north = 0
                     const float alt  // in radians, horizon = 0, vertical = pi/2
  );
  float getSunAzimuth();
  float getSunAltitude();
  void submitPSSA(unsigned surfaceIndex);
  void submitPSSA(const std::vector<unsigned> &surfaceIndices);
  void submitPSSA();
  unsigned getNumSurfaces();
  float fetchPSSA(unsigned surfaceIndex);
  std::vector<float> fetchPSSA(const std::vector<unsigned> &surfaceIndices);
  std::vector<float> fetchPSSA();
  float calculatePSSA(unsigned surfaceIndex);
  std::vector<float> calculatePSSA(const std::vector<unsigned> &surfaceIndices);
  std::vector<float> calculatePSSA();
  std::map<unsigned, float>
  calculateInteriorPSSAs(const std::vector<unsigned> &transparentSurfaceIndices,
                         const std::vector<unsigned> &interiorSurfaceIndices);
  int renderScene(unsigned surfaceIndex); // Primarily for debug purposes
  int renderInteriorScene(
      std::vector<unsigned> transparentSurfaceIndices,
      std::vector<unsigned> interiorSurfaceIndices); // Primarily for debug purposes
  VendorName getVendorName();

private:
  std::unique_ptr<PenumbraPrivate> penumbra;
};

} // namespace Pumbra

#endif /* PENUMBRA_H_ */
