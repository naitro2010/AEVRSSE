#include "GameEventHandler.h"
#include "Hooks.h"
#include <detours/detours.h>
#include <openvr.h>
#include <DirectXMath.h>
#pragma warning(disable : 4189)
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
        HMD = vr::VR_Init(&hmdError, vr::VRApplication_Background);
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
    auto orig_SetFrameBufferMatricesHook = (void (*)(void *t, uint64_t arg2)) nullptr;
    DirectX::XMMATRIX viewcache;
    DirectX::XMMATRIX projcache;
    bool copy_to_cache = true;
    bool restore_from_cache = false;
    bool replace_projection_matrix = true;
    bool renderLeft = true;
    float eyeSeparation = 0.065f*70.0f;
    float nZ = 15.0f;
    float fZ = 10000.0f;
    float frustum_scale = 0.0349f;
    void SetFrameBufferMatricesHook(void* t, uint64_t arg2) {
        DirectX::XMMATRIX* view=(DirectX::XMMATRIX*)REL::RelocationID(0, 388922).address();
        DirectX::XMMATRIX *proj = (DirectX::XMMATRIX *) (REL::RelocationID(0, 388926).address());
        DirectX::XMMATRIX *viewproj = (DirectX::XMMATRIX *) (REL::RelocationID(0, 388931).address());
        if (copy_to_cache) {
            viewcache = *view;
            projcache = *proj;
        }
        if (restore_from_cache)
        {
            *view = viewcache;
            *proj = projcache;
            *viewproj = XMMatrixMultiply(viewcache, projcache);
        }
        auto fov = 45.0f;
        if (!RE::BSGraphics::Renderer::GetSingleton()) {
            return;
        }

        
        auto size = RE::BSGraphics::Renderer::GetSingleton()->GetScreenSize();

        float aspectRatio = ((float) size.width) / ((float) size.height);
        
        if (replace_projection_matrix == true) {
            if (auto cam=RE::PlayerCamera::GetSingleton()) {
                if (auto camstate = cam->currentState) {
                    if (cam->IsInFirstPerson()) {
                        //fov = cam->firstPersonFOV * (M_PI / 180.0f);
                        fov = cam->worldFOV * (M_PI / 180.0f);
                    } else {
                        fov = cam->worldFOV*(M_PI/180.0f);
                    }
                }
            }
            float vFov = fov / aspectRatio;
            float viewHeight = 2.0f * nZ * tanf(vFov / 2.0f);
            float viewWidth = viewHeight * aspectRatio;
            float shift = renderLeft ? (eyeSeparation / 2.0f) : (-eyeSeparation / 2.0f);
            float frustum_shift = renderLeft ? (nZ * frustum_scale) : -(nZ * frustum_scale);
            float left_left = -viewWidth / 2.0f - (frustum_shift);
            float left_right = viewWidth / 2.0f - (frustum_shift);
            float left_bottom = -viewHeight / 2.0f;
            float left_top = viewHeight / 2.0f;
            DirectX::XMMATRIX oldview = *view;
            DirectX::XMMATRIX oldproj = *proj;
            DirectX::XMMATRIX oldviewproj = *viewproj;
            *view = XMMatrixMultiply(DirectX::XMMatrixTranslation(-shift, 0.0, 0.0), (*view));
            *proj = DirectX::XMMatrixPerspectiveOffCenterLH(left_left, left_right, left_bottom, left_top, nZ, fZ);
            *viewproj = XMMatrixMultiply(*view,*proj);
            
            orig_SetFrameBufferMatricesHook(t, arg2);
            *view = oldview;
            *proj = oldproj;
            *viewproj = oldviewproj;
        } else {
            return orig_SetFrameBufferMatricesHook(t, arg2);
        }
        
        
    }
    auto orig_DrawCallHook = (void (*)(void *t, float frameDelta)) nullptr;
    void DrawCallHook(void *t, float frameDelta) {
        if (replace_projection_matrix == true) {
            if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
                //auto tex=renderer->CreateRenderTexture(screenSize.width, screenSize.height);
                //REX::W32::ID3D11RenderTargetView *view;
                //renderer->GetDevice()->CreateRenderTargetView((REX::W32::ID3D11Resource*)tex->texture,nullptr,&view);
                /* if (REX::W32::D3D11_VIEWPORT *viewport = (REX::W32::D3D11_VIEWPORT *) REL::RelocationID(0, 388834).address()) {
                    if (renderLeft) {
                        viewport->topLeftX = 0.0f;
                    } else {
                        viewport->topLeftX = static_cast<float>(screenSize.width / 2.0f);
                    }

                    viewport->topLeftY = 0.0f;
                    viewport->width = static_cast<float>(screenSize.width / 2.0f);
                    viewport->height = static_cast<float>(screenSize.height);
                    viewport->minDepth = 0.0f;
                    viewport->maxDepth = 1.0f;
                    //context->RSSetViewports(1, &leftViewport);

                }*/
                float *WorldTimeFrame = (float*)REL::RelocationID(0, 410199).address();
                float *RealTimeFrame = (float *) REL::RelocationID(0, 410200).address();
                
                renderLeft = !renderLeft;
                if (renderLeft) {

                    RE::Main::GetSingleton()->freezeTime = false;
                    *WorldTimeFrame *= 2.0f;
                    *RealTimeFrame *= 2.0f;
                    orig_DrawCallHook(t, frameDelta);

                    /* if (REX::W32::D3D11_VIEWPORT *viewport = (REX::W32::D3D11_VIEWPORT *) REL::RelocationID(0, 388834).address()) {
                    if (renderLeft) {
                        viewport->topLeftX = 0.0f;
                    } else {
                        viewport->topLeftX = static_cast<float>(screenSize.width / 2.0f);
                    }

                    viewport->topLeftY = 0.0f;
                    viewport->width = static_cast<float>(screenSize.width / 2.0f);
                    viewport->height = static_cast<float>(screenSize.height);
                    viewport->minDepth = 0.0f;
                    viewport->maxDepth = 1.0f;
                    //context->RSSetViewports(1, &leftViewport);
                }*/
                    
                } else {
                    RE::Main::GetSingleton()->freezeTime = true;
                    orig_DrawCallHook(t, frameDelta);
                }
                
                /* context->RSSetViewports(1, &rightViewport);
                renderLeft = false;
                orig_DrawCallHook(t, frameDelta);
                */
                //RE::Main::GetSingleton()->freezeTime = false;
            }
        } else {
            orig_DrawCallHook(t, frameDelta);
        }
    }
    auto orig_PlayerCameraUpdate = (void (*)(RE::PlayerCamera *)) nullptr;
    
    /* void UpdatePlayerCameraHook(RE::PlayerCamera *cam) {
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
        }
        vr::HmdMatrix34_t matrix3(poses[0].mDeviceToAbsoluteTracking);
        //vr::HmdMatrix34_t matrix3(matrix2);

        if (RE::PlayerCamera::GetSingleton() != nullptr) {
            if (auto root = RE::PlayerCamera::GetSingleton()->cameraRoot) {
                if (auto state = RE::PlayerCamera::GetSingleton()->currentState) {
                    if (!root->AsNode()) {
                        return;
                    }
                    for (auto obj: root->AsNode()->GetChildren()) {
                        if (obj != nullptr) {
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

                            matrix4 = change_basis_matrix * matrix4.Transpose() * change_basis_matrix;

                            matrix4 = change_basis_matrix2 * matrix4 * change_basis_matrix2;

                            obj->world.rotate = obj->world.rotate * matrix4;
                        }
                        break;
                    }
                }
            }
        }
    }*/
    static bool patched = false;
    void GameEventHandler::onPostLoadGame() {
        if (patched == false) {
            auto version = REL::Module::get().version();
            if (version == REL::Version(1, 6, 1170, 0)) {
                orig_SetFrameBufferMatricesHook = (void (*)(void *t, uint64_t arg2)) REL::RelocationID(0, 77258).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) orig_SetFrameBufferMatricesHook, SetFrameBufferMatricesHook);
                DetourTransactionCommit(); 
                auto &trampoline = SKSE::GetTrampoline();
                SKSE::AllocTrampoline(14);
                
                orig_DrawCallHook = (void (*)(void *t, float frameDelta))
                    trampoline.write_call<5>(REL::RelocationID(0, 36544).address()+0x160, DrawCallHook);
                /* orig_PlayerCameraUpdate = (void (*)(RE::PlayerCamera *)) REL::Offset(0x8e29a0).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) orig_PlayerCameraUpdate, UpdatePlayerCameraHook);
                DetourTransactionCommit();
                */
                patched = true;
            } else if (version == REL::Version(1, 5, 97, 0)) {
                /* orig_PlayerCameraUpdate = (void (*)(RE::PlayerCamera *)) REL::Offset(0x770970).address();
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());
                DetourAttach(&(PVOID &) orig_PlayerCameraUpdate, UpdatePlayerCameraHook);
                DetourTransactionCommit();*/
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