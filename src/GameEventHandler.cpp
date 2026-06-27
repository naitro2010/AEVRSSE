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
    float eyeSeparation = 0.0214f * 70.0f;
    float frustum_scale = 0.05f;
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

        if (!RE::BSGraphics::Renderer::GetSingleton()) {
            return;
        }
        
        
        auto size = RE::BSGraphics::Renderer::GetSingleton()->GetScreenSize();

        float aspectRatio = ((float) size.width) / ((float) size.height);
        if (replace_projection_matrix == true) {
            auto original_vfov = atanf(1.0f / proj->r[1].m128_f32[1]) * 2.0f;
            auto original_nZ = proj->r[3].m128_f32[2] / (-proj->r[2].m128_f32[2]);
            auto original_fZ = ((proj->r[2].m128_f32[2] * original_nZ) / (proj->r[2].m128_f32[2]-1.0f));
            float nZ = original_nZ;
            float fZ = original_fZ;
            float fov = original_vfov*aspectRatio;
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
            *view = XMMatrixMultiply((*view), DirectX::XMMatrixTranslation(-shift, 0.0, 0.0));
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
    auto orig_SceneUpdateD = (void (*)(uint32_t a)) nullptr;
    void SceneUpdateD(uint32_t a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                return;
            } else {
                orig_SceneUpdateD(a);
            }
        }
    }
    auto orig_SceneUpdateC = (void (*)(uint32_t a)) nullptr;
    void SceneUpdateC(uint32_t a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                return;
            } else {
                orig_SceneUpdateC(a);
            }
        }
    }
    auto orig_SceneUpdateB = (void (*)(void* a)) nullptr;
    void SceneUpdateB(void* a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                orig_SceneUpdateB(a);
            } else {
                orig_SceneUpdateB(a);
            }
        } else {
        
            orig_SceneUpdateB(a);
        }
    }
    auto orig_SceneUpdate = (void (*)(void* a))nullptr;
    void SceneUpdate(void* a) {
        if (replace_projection_matrix == true) {
            if (renderLeft == true) {
                RE::BSGraphics::Renderer::GetSingleton()->Lock();
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                orig_SceneUpdate(a);
                (*(uint32_t *) REL::RelocationID(0, 411489).address()) -= 1;
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                RE::BSGraphics::Renderer::GetSingleton()->Unlock();

                return;
            } else {
                RE::BSGraphics::Renderer::GetSingleton()->Lock();
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                orig_SceneUpdate(a);
                //(*(uint32_t *) REL::RelocationID(0, 411489).address()) -= 1;
                {
                    REX::W32::D3D11_QUERY_DESC queryDesc;
                    queryDesc.query = REX::W32::D3D11_QUERY::D3D11_QUERY_EVENT;
                    queryDesc.miscFlags = 0;
                    REX::W32::ID3D11Query *pEventQuery = nullptr;
                    RE::BSGraphics::Renderer::GetSingleton()->GetDevice()->CreateQuery(&queryDesc, &pEventQuery);
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->Flush();
                    RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->End(pEventQuery);
                    while (RE::BSGraphics::Renderer::GetSingleton()->GetRendererData()->context->GetData(pEventQuery, nullptr, 0, 0) != 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                    pEventQuery->Release();
                }
                RE::BSGraphics::Renderer::GetSingleton()->Unlock();
                //orig_SceneUpdate(a);
                //(*(uint32_t *) REL::RelocationID(0, 411489).address()) += 1;
                return;
            }
        } else {
            orig_SceneUpdate(a);
        }
    }
    auto orig_DrawCallHook = (void (*)(void *t, float)) nullptr;
    void DrawCallHook(void *t, float mode) {
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
                
                

                    //RE::Main::GetSingleton()->freezeTime = false;
                
                RE::BSGraphics::Renderer::GetSingleton()->Lock();
                renderLeft = true;
                //SetFrameBufferMatricesHook(RE::BSGraphics::Renderer::GetSingleton(), 0);
                orig_DrawCallHook(t, mode);
                //SetFrameBufferMatricesHook(RE::BSGraphics::Renderer::GetSingleton(), 0);
                //RE::BSGraphics::Renderer::GetSingleton()->GetCurrentRenderWindow()->swapChain->Present(1, 0);
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
                    
                renderLeft = false;
                    
                //SetFrameBufferMatricesHook(RE::BSGraphics::Renderer::GetSingleton(), 0);
                orig_DrawCallHook(t, mode);
                //SetFrameBufferMatricesHook(RE::BSGraphics::Renderer::GetSingleton(), 0);
                //RE::BSGraphics::Renderer::GetSingleton()->GetCurrentRenderWindow()->swapChain->Present(1, 0);
                RE::BSGraphics::Renderer::GetSingleton()->Unlock();

                
                /* context->RSSetViewports(1, &rightViewport);
                renderLeft = false;
                orig_DrawCallHook(t, frameDelta);
                */
                //RE::Main::GetSingleton()->freezeTime = false;
            }
        } else {
            orig_DrawCallHook(t, mode);
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
                orig_DrawCallHook = (void (*)(void *t, float delta))
                    trampoline.write_call<5>(REL::RelocationID(0, 36564).address()+0xa97, DrawCallHook);
                SKSE::AllocTrampoline(14);
                orig_SceneUpdate =
                    (void (*)(void* a)) trampoline.write_call<5>(REL::RelocationID(0, 36555).address() + 0x601, SceneUpdate);
                SKSE::AllocTrampoline(14);
                orig_SceneUpdateB =
                    (void (*)(void* a)) trampoline.write_call<5>(REL::RelocationID(0, 36555).address() + 0x5f0, SceneUpdateB);
                /* SKSE::AllocTrampoline(14);
                orig_SceneUpdateC =
                    (void (*)(uint32_t a)) trampoline.write_branch<5>(REL::RelocationID(0, 36555).address() + 0x659, SceneUpdateC);*/
                //SKSE::AllocTrampoline(14);
                /* orig_SceneUpdateD =
                    (void (*)(uint32_t a)) trampoline.write_call<5>(REL::RelocationID(0, 36555).address() + 0x2ea, SceneUpdateD);*/
                //orig_SceneUpdate = (void (*)(void *t)) trampoline.write_call<5>(
                //    REL::RelocationID(0, 36555).address() + 0x5f0, SceneUpdate);
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