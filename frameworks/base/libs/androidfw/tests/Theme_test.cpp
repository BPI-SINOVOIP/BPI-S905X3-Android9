/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "androidfw/AssetManager2.h"

#include "android-base/logging.h"

#include "TestHelpers.h"
#include "androidfw/ResourceUtils.h"
#include "data/lib_one/R.h"
#include "data/libclient/R.h"
#include "data/styles/R.h"
#include "data/system/R.h"

namespace app = com::android::app;
namespace lib_one = com::android::lib_one;
namespace libclient = com::android::libclient;

namespace android {

class ThemeTest : public ::testing::Test {
 public:
  void SetUp() override {
    system_assets_ = ApkAssets::Load(GetTestDataPath() + "/system/system.apk", true /*system*/);
    ASSERT_NE(nullptr, system_assets_);

    style_assets_ = ApkAssets::Load(GetTestDataPath() + "/styles/styles.apk");
    ASSERT_NE(nullptr, style_assets_);

    libclient_assets_ = ApkAssets::Load(GetTestDataPath() + "/libclient/libclient.apk");
    ASSERT_NE(nullptr, libclient_assets_);

    lib_one_assets_ = ApkAssets::Load(GetTestDataPath() + "/lib_one/lib_one.apk");
    ASSERT_NE(nullptr, lib_one_assets_);

    lib_two_assets_ = ApkAssets::Load(GetTestDataPath() + "/lib_two/lib_two.apk");
    ASSERT_NE(nullptr, lib_two_assets_);
  }

 protected:
  std::unique_ptr<const ApkAssets> system_assets_;
  std::unique_ptr<const ApkAssets> style_assets_;
  std::unique_ptr<const ApkAssets> libclient_assets_;
  std::unique_ptr<const ApkAssets> lib_one_assets_;
  std::unique_ptr<const ApkAssets> lib_two_assets_;
};

TEST_F(ThemeTest, EmptyTheme) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  EXPECT_EQ(0u, theme->GetChangingConfigurations());
  EXPECT_EQ(&assetmanager, theme->GetAssetManager());

  Res_value value;
  uint32_t flags;
  EXPECT_EQ(kInvalidCookie, theme->GetAttribute(app::R::attr::attr_one, &value, &flags));
}

TEST_F(ThemeTest, SingleThemeNoParent) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleOne));

  Res_value value;
  uint32_t flags;
  ApkAssetsCookie cookie;

  cookie = theme->GetAttribute(app::R::attr::attr_one, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(1u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  cookie = theme->GetAttribute(app::R::attr::attr_two, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(2u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);
}

TEST_F(ThemeTest, SingleThemeWithParent) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleTwo));

  Res_value value;
  uint32_t flags;
  ApkAssetsCookie cookie;

  cookie = theme->GetAttribute(app::R::attr::attr_one, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(1u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  cookie = theme->GetAttribute(app::R::attr::attr_two, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_STRING, value.dataType);
  EXPECT_EQ(0, cookie);
  EXPECT_EQ(std::string("string"),
            GetStringFromPool(assetmanager.GetStringPoolForCookie(0), value.data));
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // This attribute should point to an attr_indirect, so the result should be 3.
  cookie = theme->GetAttribute(app::R::attr::attr_three, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(3u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);
}

TEST_F(ThemeTest, TryToUseBadResourceId) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleTwo));

  Res_value value;
  uint32_t flags;
  ASSERT_EQ(kInvalidCookie, theme->GetAttribute(0x7f000001, &value, &flags));
}

TEST_F(ThemeTest, MultipleThemesOverlaidNotForce) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleTwo));
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleThree));

  Res_value value;
  uint32_t flags;
  ApkAssetsCookie cookie;

  // attr_one is still here from the base.
  cookie = theme->GetAttribute(app::R::attr::attr_one, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(1u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // check for the new attr_six
  cookie = theme->GetAttribute(app::R::attr::attr_six, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(6u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // check for the old attr_five (force=true was not used).
  cookie = theme->GetAttribute(app::R::attr::attr_five, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_REFERENCE, value.dataType);
  EXPECT_EQ(app::R::string::string_one, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);
}

TEST_F(ThemeTest, MultipleThemesOverlaidForced) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleTwo));
  ASSERT_TRUE(theme->ApplyStyle(app::R::style::StyleThree, true /* force */));

  Res_value value;
  uint32_t flags;
  ApkAssetsCookie cookie;

  // attr_one is still here from the base.
  cookie = theme->GetAttribute(app::R::attr::attr_one, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(1u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // check for the new attr_six
  cookie = theme->GetAttribute(app::R::attr::attr_six, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(6u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // check for the new attr_five (force=true was used).
  cookie = theme->GetAttribute(app::R::attr::attr_five, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(5u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);
}

TEST_F(ThemeTest, ResolveDynamicAttributesAndReferencesToSharedLibrary) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets(
      {lib_two_assets_.get(), lib_one_assets_.get(), libclient_assets_.get()});

  std::unique_ptr<Theme> theme = assetmanager.NewTheme();
  ASSERT_TRUE(theme->ApplyStyle(libclient::R::style::Theme, false /*force*/));

  Res_value value;
  uint32_t flags;
  ApkAssetsCookie cookie;

  // The attribute should be resolved to the final value.
  cookie = theme->GetAttribute(libclient::R::attr::foo, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(700u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // The reference should be resolved to a TYPE_REFERENCE.
  cookie = theme->GetAttribute(libclient::R::attr::bar, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_REFERENCE, value.dataType);

  // lib_one is assigned package ID 0x03.
  EXPECT_EQ(3u, get_package_id(value.data));
  EXPECT_EQ(get_type_id(lib_one::R::string::foo), get_type_id(value.data));
  EXPECT_EQ(get_entry_id(lib_one::R::string::foo), get_entry_id(value.data));
}

TEST_F(ThemeTest, CopyThemeSameAssetManager) {
  AssetManager2 assetmanager;
  assetmanager.SetApkAssets({style_assets_.get()});

  std::unique_ptr<Theme> theme_one = assetmanager.NewTheme();
  ASSERT_TRUE(theme_one->ApplyStyle(app::R::style::StyleOne));

  Res_value value;
  uint32_t flags;
  ApkAssetsCookie cookie;

  // attr_one is still here from the base.
  cookie = theme_one->GetAttribute(app::R::attr::attr_one, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(1u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);

  // attr_six is not here.
  EXPECT_EQ(kInvalidCookie, theme_one->GetAttribute(app::R::attr::attr_six, &value, &flags));

  std::unique_ptr<Theme> theme_two = assetmanager.NewTheme();
  ASSERT_TRUE(theme_two->ApplyStyle(app::R::style::StyleThree));

  // Copy the theme to theme_one.
  ASSERT_TRUE(theme_one->SetTo(*theme_two));

  // Clear theme_two to make sure we test that there WAS a copy.
  theme_two->Clear();

  // attr_one is now not here.
  EXPECT_EQ(kInvalidCookie, theme_one->GetAttribute(app::R::attr::attr_one, &value, &flags));

  // attr_six is now here because it was copied.
  cookie = theme_one->GetAttribute(app::R::attr::attr_six, &value, &flags);
  ASSERT_NE(kInvalidCookie, cookie);
  EXPECT_EQ(Res_value::TYPE_INT_DEC, value.dataType);
  EXPECT_EQ(6u, value.data);
  EXPECT_EQ(static_cast<uint32_t>(ResTable_typeSpec::SPEC_PUBLIC), flags);
}

TEST_F(ThemeTest, OnlyCopySystemThemeWhenAssetManagersDiffer) {
  AssetManager2 assetmanager_one;
  assetmanager_one.SetApkAssets({system_assets_.get(), style_assets_.get()});

  AssetManager2 assetmanager_two;
  assetmanager_two.SetApkAssets({system_assets_.get(), style_assets_.get()});

  auto theme_one = assetmanager_one.NewTheme();
  ASSERT_TRUE(theme_one->ApplyStyle(app::R::style::StyleOne));

  auto theme_two = assetmanager_two.NewTheme();
  ASSERT_TRUE(theme_two->ApplyStyle(R::style::Theme_One));
  ASSERT_TRUE(theme_two->ApplyStyle(app::R::style::StyleTwo));

  EXPECT_TRUE(theme_one->SetTo(*theme_two));

  Res_value value;
  uint32_t flags;

  // No app resources.
  EXPECT_EQ(kInvalidCookie, theme_one->GetAttribute(app::R::attr::attr_one, &value, &flags));

  // Only system.
  EXPECT_NE(kInvalidCookie, theme_one->GetAttribute(R::attr::foreground, &value, &flags));
}

}  // namespace android
