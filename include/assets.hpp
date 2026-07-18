#pragma once

#include "metacore/shared/assets.hpp"

#define DECLARE_ASSET(name, binary)       \
    const IncludedAsset name {            \
        Externs::_binary_##binary##_start, \
        Externs::_binary_##binary##_end    \
    };

#define DECLARE_ASSET_NS(namespaze, name, binary) \
    namespace namespaze { DECLARE_ASSET(name, binary) }

namespace IncludedAssets {
    namespace Externs {
        extern "C" uint8_t _binary_Resources_Colors_png_start[];
        extern "C" uint8_t _binary_Resources_Colors_png_end[];
        extern "C" uint8_t _binary_Resources_CustomLevelsCover_png_start[];
        extern "C" uint8_t _binary_Resources_CustomLevelsCover_png_end[];
        extern "C" uint8_t _binary_Resources_CustomLevelsCoverEvil_png_start[];
        extern "C" uint8_t _binary_Resources_CustomLevelsCoverEvil_png_end[];
        extern "C" uint8_t _binary_Resources_CustomWIPLevelsCover_png_start[];
        extern "C" uint8_t _binary_Resources_CustomWIPLevelsCover_png_end[];
        extern "C" uint8_t _binary_Resources_DeleteIcon_png_start[];
        extern "C" uint8_t _binary_Resources_DeleteIcon_png_end[];
        extern "C" uint8_t _binary_Resources_Environment_png_start[];
        extern "C" uint8_t _binary_Resources_Environment_png_end[];
        extern "C" uint8_t _binary_Resources_ExtraDiffsIcon_png_start[];
        extern "C" uint8_t _binary_Resources_ExtraDiffsIcon_png_end[];
        extern "C" uint8_t _binary_Resources_FolderIcon_png_start[];
        extern "C" uint8_t _binary_Resources_FolderIcon_png_end[];
        extern "C" uint8_t _binary_Resources_GreenCheck_png_start[];
        extern "C" uint8_t _binary_Resources_GreenCheck_png_end[];
        extern "C" uint8_t _binary_Resources_Info_png_start[];
        extern "C" uint8_t _binary_Resources_Info_png_end[];
        extern "C" uint8_t _binary_Resources_Lawless_png_start[];
        extern "C" uint8_t _binary_Resources_Lawless_png_end[];
        extern "C" uint8_t _binary_Resources_Lightshow_png_start[];
        extern "C" uint8_t _binary_Resources_Lightshow_png_end[];
        extern "C" uint8_t _binary_Resources_MissingChar_png_start[];
        extern "C" uint8_t _binary_Resources_MissingChar_png_end[];
        extern "C" uint8_t _binary_Resources_OneSaber_png_start[];
        extern "C" uint8_t _binary_Resources_OneSaber_png_end[];
        extern "C" uint8_t _binary_Resources_RedX_png_start[];
        extern "C" uint8_t _binary_Resources_RedX_png_end[];
        extern "C" uint8_t _binary_Resources_Standard_png_start[];
        extern "C" uint8_t _binary_Resources_Standard_png_end[];
        extern "C" uint8_t _binary_Resources_Warning_png_start[];
        extern "C" uint8_t _binary_Resources_Warning_png_end[];
        extern "C" uint8_t _binary_Resources_YellowCheck_png_start[];
        extern "C" uint8_t _binary_Resources_YellowCheck_png_end[];
        extern "C" uint8_t _binary_Resources_YellowX_png_start[];
        extern "C" uint8_t _binary_Resources_YellowX_png_end[];
        extern "C" uint8_t _binary_Resources_pixel_png_start[];
        extern "C" uint8_t _binary_Resources_pixel_png_end[];
        extern "C" uint8_t _binary_colors_bsml_start[];
        extern "C" uint8_t _binary_colors_bsml_end[];
        extern "C" uint8_t _binary_deletemodal_bsml_start[];
        extern "C" uint8_t _binary_deletemodal_bsml_end[];
        extern "C" uint8_t _binary_requirements_bsml_start[];
        extern "C" uint8_t _binary_requirements_bsml_end[];
    }

    // Resources/Colors.png
    DECLARE_ASSET_NS(Resources, Colors_png, Resources_Colors_png);
    // Resources/CustomLevelsCover.png
    DECLARE_ASSET_NS(Resources, CustomLevelsCover_png, Resources_CustomLevelsCover_png);
    // Resources/CustomLevelsCoverEvil.png
    DECLARE_ASSET_NS(Resources, CustomLevelsCoverEvil_png, Resources_CustomLevelsCoverEvil_png);
    // Resources/CustomWIPLevelsCover.png
    DECLARE_ASSET_NS(Resources, CustomWIPLevelsCover_png, Resources_CustomWIPLevelsCover_png);
    // Resources/DeleteIcon.png
    DECLARE_ASSET_NS(Resources, DeleteIcon_png, Resources_DeleteIcon_png);
    // Resources/Environment.png
    DECLARE_ASSET_NS(Resources, Environment_png, Resources_Environment_png);
    // Resources/ExtraDiffsIcon.png
    DECLARE_ASSET_NS(Resources, ExtraDiffsIcon_png, Resources_ExtraDiffsIcon_png);
    // Resources/FolderIcon.png
    DECLARE_ASSET_NS(Resources, FolderIcon_png, Resources_FolderIcon_png);
    // Resources/GreenCheck.png
    DECLARE_ASSET_NS(Resources, GreenCheck_png, Resources_GreenCheck_png);
    // Resources/Info.png
    DECLARE_ASSET_NS(Resources, Info_png, Resources_Info_png);
    // Resources/Lawless.png
    DECLARE_ASSET_NS(Resources, Lawless_png, Resources_Lawless_png);
    // Resources/Lightshow.png
    DECLARE_ASSET_NS(Resources, Lightshow_png, Resources_Lightshow_png);
    // Resources/MissingChar.png
    DECLARE_ASSET_NS(Resources, MissingChar_png, Resources_MissingChar_png);
    // Resources/OneSaber.png
    DECLARE_ASSET_NS(Resources, OneSaber_png, Resources_OneSaber_png);
    // Resources/RedX.png
    DECLARE_ASSET_NS(Resources, RedX_png, Resources_RedX_png);
    // Resources/Standard.png
    DECLARE_ASSET_NS(Resources, Standard_png, Resources_Standard_png);
    // Resources/Warning.png
    DECLARE_ASSET_NS(Resources, Warning_png, Resources_Warning_png);
    // Resources/YellowCheck.png
    DECLARE_ASSET_NS(Resources, YellowCheck_png, Resources_YellowCheck_png);
    // Resources/YellowX.png
    DECLARE_ASSET_NS(Resources, YellowX_png, Resources_YellowX_png);
    // Resources/pixel.png
    DECLARE_ASSET_NS(Resources, pixel_png, Resources_pixel_png);
    // colors.bsml
    DECLARE_ASSET(colors_bsml, colors_bsml);
    // deletemodal.bsml
    DECLARE_ASSET(deletemodal_bsml, deletemodal_bsml);
    // requirements.bsml
    DECLARE_ASSET(requirements_bsml, requirements_bsml);
}
