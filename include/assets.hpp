#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-narrowing"
#pragma clang diagnostic ignored "-Wc23-extensions"

#include "beatsaber-hook/shared/arrayw.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace IncludedAssets {
    namespace raw {
        // --- text assets (null-suffixed) ---
        inline constexpr unsigned char colors_bsml[] = {
            #embed "../assets/colors.bsml" suffix(, 0)
        };
        inline constexpr unsigned char deletemodal_bsml[] = {
            #embed "../assets/deletemodal.bsml" suffix(, 0)
        };
        inline constexpr unsigned char requirements_bsml[] = {
            #embed "../assets/requirements.bsml" suffix(, 0)
        };

        // --- image assets ---
        inline constexpr uint8_t Colors_png[] = {
            #embed "../assets/Resources/Colors.png"
        };
        inline constexpr uint8_t CustomLevelsCover_png[] = {
            #embed "../assets/Resources/CustomLevelsCover.png"
        };
        inline constexpr uint8_t CustomLevelsCoverEvil_png[] = {
            #embed "../assets/Resources/CustomLevelsCoverEvil.png"
        };
        inline constexpr uint8_t CustomWIPLevelsCover_png[] = {
            #embed "../assets/Resources/CustomWIPLevelsCover.png"
        };
        inline constexpr uint8_t DeleteIcon_png[] = {
            #embed "../assets/Resources/DeleteIcon.png"
        };
        inline constexpr uint8_t Environment_png[] = {
            #embed "../assets/Resources/Environment.png"
        };
        inline constexpr uint8_t ExtraDiffsIcon_png[] = {
            #embed "../assets/Resources/ExtraDiffsIcon.png"
        };
        inline constexpr uint8_t FolderIcon_png[] = {
            #embed "../assets/Resources/FolderIcon.png"
        };
        inline constexpr uint8_t GreenCheck_png[] = {
            #embed "../assets/Resources/GreenCheck.png"
        };
        inline constexpr uint8_t Info_png[] = {
            #embed "../assets/Resources/Info.png"
        };
        inline constexpr uint8_t Lawless_png[] = {
            #embed "../assets/Resources/Lawless.png"
        };
        inline constexpr uint8_t Lightshow_png[] = {
            #embed "../assets/Resources/Lightshow.png"
        };
        inline constexpr uint8_t MissingChar_png[] = {
            #embed "../assets/Resources/MissingChar.png"
        };
        inline constexpr uint8_t OneSaber_png[] = {
            #embed "../assets/Resources/OneSaber.png"
        };
        inline constexpr uint8_t RedX_png[] = {
            #embed "../assets/Resources/RedX.png"
        };
        inline constexpr uint8_t Standard_png[] = {
            #embed "../assets/Resources/Standard.png"
        };
        inline constexpr uint8_t Warning_png[] = {
            #embed "../assets/Resources/Warning.png"
        };
        inline constexpr uint8_t YellowCheck_png[] = {
            #embed "../assets/Resources/YellowCheck.png"
        };
        inline constexpr uint8_t YellowX_png[] = {
            #embed "../assets/Resources/YellowX.png"
        };
        inline constexpr uint8_t pixel_png[] = {
            #embed "../assets/Resources/pixel.png"
        };
    }

#pragma clang diagnostic pop

    // helper: view over a null-suffixed raw text array, terminator excluded
    template <std::size_t N>
    inline std::string_view text(const unsigned char (&arr)[N]) {
        return {reinterpret_cast<const char*>(arr), N - 1};
    }

    inline std::string_view const colors_bsml = text(raw::colors_bsml);
    inline std::string_view const deletemodal_bsml = text(raw::deletemodal_bsml);
    inline std::string_view const requirements_bsml = text(raw::requirements_bsml);

    namespace Resources {
        inline constexpr std::span<const uint8_t> Colors_png{raw::Colors_png};
        inline constexpr std::span<const uint8_t> CustomLevelsCover_png{raw::CustomLevelsCover_png};
        inline constexpr std::span<const uint8_t> CustomLevelsCoverEvil_png{raw::CustomLevelsCoverEvil_png};
        inline constexpr std::span<const uint8_t> CustomWIPLevelsCover_png{raw::CustomWIPLevelsCover_png};
        inline constexpr std::span<const uint8_t> DeleteIcon_png{raw::DeleteIcon_png};
        inline constexpr std::span<const uint8_t> Environment_png{raw::Environment_png};
        inline constexpr std::span<const uint8_t> ExtraDiffsIcon_png{raw::ExtraDiffsIcon_png};
        inline constexpr std::span<const uint8_t> FolderIcon_png{raw::FolderIcon_png};
        inline constexpr std::span<const uint8_t> GreenCheck_png{raw::GreenCheck_png};
        inline constexpr std::span<const uint8_t> Info_png{raw::Info_png};
        inline constexpr std::span<const uint8_t> Lawless_png{raw::Lawless_png};
        inline constexpr std::span<const uint8_t> Lightshow_png{raw::Lightshow_png};
        inline constexpr std::span<const uint8_t> MissingChar_png{raw::MissingChar_png};
        inline constexpr std::span<const uint8_t> OneSaber_png{raw::OneSaber_png};
        inline constexpr std::span<const uint8_t> RedX_png{raw::RedX_png};
        inline constexpr std::span<const uint8_t> Standard_png{raw::Standard_png};
        inline constexpr std::span<const uint8_t> Warning_png{raw::Warning_png};
        inline constexpr std::span<const uint8_t> YellowCheck_png{raw::YellowCheck_png};
        inline constexpr std::span<const uint8_t> YellowX_png{raw::YellowX_png};
        inline constexpr std::span<const uint8_t> pixel_png{raw::pixel_png};
    }
}
