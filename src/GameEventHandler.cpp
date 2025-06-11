#include "GameEventHandler.h"
#include "Hooks.h"
#include <detours/detours.h>
#include <openvr.h>
#define M_PI 3.1415926535897932384626433832795f
namespace plugin {
    void GameEventHandler::onLoad() {
        logger::info("onLoad()");
        Hooks::install();
    }

    void GameEventHandler::onPostLoad() {
        logger::info("onPostLoad()");
    }

    vr::IVRSystem *HMD;
    void GameEventHandler::onPostPostLoad() {
        vr::HmdError hmdError;
        HMD=vr::VR_Init(&hmdError, vr::VRApplication_Background);
        if (hmdError != vr::VRInitError_None) {
            HMD = nullptr;
            logger::error("please launch steamvr if you want head tracking");
        }
        logger::info("onPostPostLoad()");
    }

    void GameEventHandler::onInputLoaded() {
        logger::info("onInputLoaded()");
    }

    void GameEventHandler::onDataLoaded() {
        logger::info("onDataLoaded()");
    }

    void GameEventHandler::onNewGame() {
        logger::info("onNewGame()");
    }

    void GameEventHandler::onPreLoadGame() {
        logger::info("onPreLoadGame()");
    }
    auto orig_PlayerCameraUpdate = (void (*)(RE::PlayerCamera *)) nullptr;
    void UpdatePlayerCameraHook(RE::PlayerCamera* cam) {
        orig_PlayerCameraUpdate(cam);
        unsigned int unDevice = 0;
        if (!HMD) {
            return;
        }
        if (!HMD->IsTrackedDeviceConnected(unDevice))
            return;
        vr::TrackedDevicePose_t poses[8];
        HMD->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseRawAndUncalibrated, 0.0f, poses, 8);
        if (poses[0].bDeviceIsConnected == false) {
            return;
        }
        if (poses[0].bPoseIsValid == false) {
            return;
        }       vr::HmdMatrix34_t matrix3(poses[0].mDeviceToAbsoluteTracking);
       //vr::HmdMatrix34_t matrix3(matrix2);
       
       if (RE::PlayerCamera::GetSingleton() != nullptr) {
           if (auto root=RE::PlayerCamera::GetSingleton()->cameraRoot) {
               if (auto state = RE::PlayerCamera::GetSingleton()->currentState)
               {
                   if (!root->AsNode()) {
                       return;
                   }
                   for (auto obj: root->AsNode()->GetChildren()) {
                       if (obj != nullptr) 
                       {
                           RE::NiPoint3 angle;
                           RE::NiMatrix3 matrix4;
                           matrix4.entry[0][0] = matrix3.m[0][0];
                           matrix4.entry[0][1] = matrix3.m[1][0];
                           matrix4.entry[0][2] = matrix3.m[2][0];
                           matrix4.entry[1][0] = matrix3.m[0][1];
                           matrix4.entry[1][1] = matrix3.m[1][1];
                           matrix4.entry[1][2] = matrix3.m[2][1];
                           matrix4.entry[2][0] = matrix3.m[0][2];
                           matrix4.entry[2][1] = matrix3.m[1][2];
                           matrix4.entry[2][2] = matrix3.m[2][2];
                           
                           RE::NiMatrix3 change_basis_matrix;
                           change_basis_matrix.entry[0][0] = 1.0;
                           change_basis_matrix.entry[0][2] = 0.0;
                           change_basis_matrix.entry[1][1] = 1.0;
                           change_basis_matrix.entry[2][0] = 0.0;
                           change_basis_matrix.entry[2][2] = -1.0;
                           
                           RE::NiMatrix3 change_basis_matrix2;
                           change_basis_matrix2.entry[0][0] = 0.0;
                           change_basis_matrix2.entry[0][2] = 1.0;
                           change_basis_matrix2.entry[1][1] = 1.0;
                           change_basis_matrix2.entry[2][0] = 1.0;
                           change_basis_matrix2.entry[2][2] = 0.0;

                           matrix4 = change_basis_matrix * matrix4.Transpose() *change_basis_matrix;

                           matrix4 = change_basis_matrix2 * matrix4 *change_basis_matrix2;

                 
                           obj->world.rotate = obj->world.rotate * matrix4;
                          
                       }
                       break;
                   }
               }
           }
       }
   }
    static bool patched = false;
    void GameEventHandler::onPostLoadGame() {
        if (patched == false) {
            auto version = REL::Module::get().version();
            if (version == REL::Version(1, 6, 1170,0)) {
                orig_PlayerCameraUpdate = (void (*)(RE::PlayerCamera *)) REL::Offset(0x8e29a0).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) orig_PlayerCameraUpdate, UpdatePlayerCameraHook);
                DetourTransactionCommit();
                patched = true;
            }
        }
        logger::info("onPostLoadGame()");
    }

    void GameEventHandler::onSaveGame() {
        logger::info("onSaveGame()");
    }

    void GameEventHandler::onDeleteGame() {
        logger::info("onDeleteGame()");
    }
}  // namespace plugin