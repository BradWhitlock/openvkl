// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <imgui.h>
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include <ospray/ospray_testing/ospray_testing.h>
#include <volley/volley.h>

using namespace ospcommon;

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  // OSPError initError = ospInit(&argc, argv);

  // if (initError != OSP_NO_ERROR)
  //   return initError;

  // For now, default to the volley device
  OSPDevice volleyDevice = ospNewDevice("scalar_volley_device");
  ospDeviceCommit(volleyDevice);
  ospSetCurrentDevice(volleyDevice);

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the "world" model which will contain all of our geometries / volumes
  OSPModel world = ospNewModel();

  // add in generated volume and transfer function
  OSPTestingVolume test_data =
      ospTestingNewVolume("volley_simple_procedural_volume");

  OSPTransferFunction tfn =
      ospTestingNewTransferFunction(test_data.voxelRange, "jet");

  ospSetObject(test_data.volume, "transferFunction", tfn);
  ospCommit(test_data.volume);

  ospAddVolume(world, test_data.volume);
  ospRelease(test_data.volume);
  ospRelease(tfn);

  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("scivis");

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{512, 512},
                           reinterpret_cast<box3f &>(test_data.bounds),
                           world,
                           renderer));

  glfwOSPRayWindow->registerImGuiCallback([=]() {
    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      ospCommit(renderer);
    }

    static float samplingRate = 1.f;
    if (ImGui::SliderFloat("samplingRate", &samplingRate, 0.01f, 2.f)) {
      ospSet1f(test_data.volume, "samplingRate", samplingRate);
      ospCommit(test_data.volume);
    }

    static VLYSamplingType vlySamplingType = VLY_SAMPLE_LINEAR;
    if (ImGui::RadioButton("Linear sampling",
                           vlySamplingType == VLY_SAMPLE_LINEAR)) {
      vlySamplingType = VLY_SAMPLE_LINEAR;

      ospSet1i(test_data.volume, "vlySamplingType", vlySamplingType);
      ospCommit(test_data.volume);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Nearest sampling",
                           vlySamplingType == VLY_SAMPLE_NEAREST)) {
      vlySamplingType = VLY_SAMPLE_NEAREST;

      ospSet1i(test_data.volume, "vlySamplingType", vlySamplingType);
      ospCommit(test_data.volume);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
