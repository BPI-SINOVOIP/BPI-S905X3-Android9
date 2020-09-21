/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "link/TableMerger.h"

#include "filter/ConfigFilter.h"
#include "io/FileSystem.h"
#include "test/Test.h"

using ::aapt::test::ValueEq;
using ::testing::Contains;
using ::testing::Eq;
using ::testing::Field;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::StrEq;
using ::testing::UnorderedElementsAreArray;

namespace aapt {

struct TableMergerTest : public ::testing::Test {
  std::unique_ptr<IAaptContext> context_;

  void SetUp() override {
    context_ =
        test::ContextBuilder()
            // We are compiling this package.
            .SetCompilationPackage("com.app.a")

            // Merge all packages that have this package ID.
            .SetPackageId(0x7f)

            // Mangle all packages that do not have this package name.
            .SetNameManglerPolicy(NameManglerPolicy{"com.app.a", {"com.app.b"}})

            .Build();
  }
};

TEST_F(TableMergerTest, SimpleMerge) {
  std::unique_ptr<ResourceTable> table_a =
      test::ResourceTableBuilder()
          .SetPackageId("com.app.a", 0x7f)
          .AddReference("com.app.a:id/foo", "com.app.a:id/bar")
          .AddReference("com.app.a:id/bar", "com.app.b:id/foo")
          .AddValue(
              "com.app.a:styleable/view",
              test::StyleableBuilder().AddItem("com.app.b:id/foo").Build())
          .Build();

  std::unique_ptr<ResourceTable> table_b = test::ResourceTableBuilder()
                                               .SetPackageId("com.app.b", 0x7f)
                                               .AddSimple("com.app.b:id/foo")
                                               .Build();

  ResourceTable final_table;
  TableMerger merger(context_.get(), &final_table, TableMergerOptions{});

  ASSERT_TRUE(merger.Merge({}, table_a.get(), false /*overlay*/));
  ASSERT_TRUE(merger.MergeAndMangle({}, "com.app.b", table_b.get()));

  EXPECT_TRUE(merger.merged_packages().count("com.app.b") != 0);

  // Entries from com.app.a should not be mangled.
  EXPECT_TRUE(final_table.FindResource(test::ParseNameOrDie("com.app.a:id/foo")));
  EXPECT_TRUE(final_table.FindResource(test::ParseNameOrDie("com.app.a:id/bar")));
  EXPECT_TRUE(final_table.FindResource(test::ParseNameOrDie("com.app.a:styleable/view")));

  // The unmangled name should not be present.
  EXPECT_FALSE(final_table.FindResource(test::ParseNameOrDie("com.app.b:id/foo")));

  // Look for the mangled name.
  EXPECT_TRUE(final_table.FindResource(test::ParseNameOrDie("com.app.a:id/com.app.b$foo")));
}

TEST_F(TableMergerTest, MergeFile) {
  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ResourceFile file_desc;
  file_desc.config = test::ParseConfigOrDie("hdpi-v4");
  file_desc.name = test::ParseNameOrDie("layout/main");
  file_desc.source = Source("res/layout-hdpi/main.xml");
  test::TestFile test_file("path/to/res/layout-hdpi/main.xml.flat");

  ASSERT_TRUE(merger.MergeFile(file_desc, false /*overlay*/, &test_file));

  FileReference* file = test::GetValueForConfig<FileReference>(
      &final_table, "com.app.a:layout/main", test::ParseConfigOrDie("hdpi-v4"));
  ASSERT_THAT(file, NotNull());
  EXPECT_EQ(std::string("res/layout-hdpi-v4/main.xml"), *file->path);
}

TEST_F(TableMergerTest, MergeFileOverlay) {
  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ResourceFile file_desc;
  file_desc.name = test::ParseNameOrDie("xml/foo");
  test::TestFile file_a("path/to/fileA.xml.flat");
  test::TestFile file_b("path/to/fileB.xml.flat");

  ASSERT_TRUE(merger.MergeFile(file_desc, false /*overlay*/, &file_a));
  ASSERT_TRUE(merger.MergeFile(file_desc, true /*overlay*/, &file_b));
}

TEST_F(TableMergerTest, MergeFileReferences) {
  test::TestFile file_a("res/xml/file.xml");
  test::TestFile file_b("res/xml/file.xml");

  std::unique_ptr<ResourceTable> table_a =
      test::ResourceTableBuilder()
          .SetPackageId("com.app.a", 0x7f)
          .AddFileReference("com.app.a:xml/file", "res/xml/file.xml", &file_a)
          .Build();
  std::unique_ptr<ResourceTable> table_b =
      test::ResourceTableBuilder()
          .SetPackageId("com.app.b", 0x7f)
          .AddFileReference("com.app.b:xml/file", "res/xml/file.xml", &file_b)
          .Build();

  ResourceTable final_table;
  TableMerger merger(context_.get(), &final_table, TableMergerOptions{});

  ASSERT_TRUE(merger.Merge({}, table_a.get(), false /*overlay*/));
  ASSERT_TRUE(merger.MergeAndMangle({}, "com.app.b", table_b.get()));

  FileReference* f = test::GetValue<FileReference>(&final_table, "com.app.a:xml/file");
  ASSERT_THAT(f, NotNull());
  EXPECT_THAT(*f->path, StrEq("res/xml/file.xml"));
  EXPECT_THAT(f->file, Eq(&file_a));

  f = test::GetValue<FileReference>(&final_table, "com.app.a:xml/com.app.b$file");
  ASSERT_THAT(f, NotNull());
  EXPECT_THAT(*f->path, StrEq("res/xml/com.app.b$file.xml"));
  EXPECT_THAT(f->file, Eq(&file_b));
}

TEST_F(TableMergerTest, OverrideResourceWithOverlay) {
  std::unique_ptr<ResourceTable> base =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x00)
          .AddValue("bool/foo", ResourceUtils::TryParseBool("true"))
          .Build();
  std::unique_ptr<ResourceTable> overlay =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x00)
          .AddValue("bool/foo", ResourceUtils::TryParseBool("false"))
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, base.get(), false /*overlay*/));
  ASSERT_TRUE(merger.Merge({}, overlay.get(), true /*overlay*/));

  BinaryPrimitive* foo = test::GetValue<BinaryPrimitive>(&final_table, "com.app.a:bool/foo");
  ASSERT_THAT(foo,
              Pointee(Field(&BinaryPrimitive::value, Field(&android::Res_value::data, Eq(0u)))));
}

TEST_F(TableMergerTest, OverrideSameResourceIdsWithOverlay) {
  std::unique_ptr<ResourceTable> base =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", ResourceId(0x7f, 0x01, 0x0001), Visibility::Level::kPublic)
          .Build();
  std::unique_ptr<ResourceTable> overlay =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", ResourceId(0x7f, 0x01, 0x0001), Visibility::Level::kPublic)
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, base.get(), false /*overlay*/));
  ASSERT_TRUE(merger.Merge({}, overlay.get(), true /*overlay*/));
}

TEST_F(TableMergerTest, FailToOverrideConflictingTypeIdsWithOverlay) {
  std::unique_ptr<ResourceTable> base =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", ResourceId(0x7f, 0x01, 0x0001), Visibility::Level::kPublic)
          .Build();
  std::unique_ptr<ResourceTable> overlay =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", ResourceId(0x7f, 0x02, 0x0001), Visibility::Level::kPublic)
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, base.get(), false /*overlay*/));
  ASSERT_FALSE(merger.Merge({}, overlay.get(), true /*overlay*/));
}

TEST_F(TableMergerTest, FailToOverrideConflictingEntryIdsWithOverlay) {
  std::unique_ptr<ResourceTable> base =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", ResourceId(0x7f, 0x01, 0x0001), Visibility::Level::kPublic)
          .Build();
  std::unique_ptr<ResourceTable> overlay =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", ResourceId(0x7f, 0x01, 0x0002), Visibility::Level::kPublic)
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, base.get(), false /*overlay*/));
  ASSERT_FALSE(merger.Merge({}, overlay.get(), true /*overlay*/));
}

TEST_F(TableMergerTest, MergeAddResourceFromOverlay) {
  std::unique_ptr<ResourceTable> table_a =
      test::ResourceTableBuilder().SetPackageId("", 0x7f).Build();
  std::unique_ptr<ResourceTable> table_b =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .SetSymbolState("bool/foo", {}, Visibility::Level::kUndefined, true /*allow new overlay*/)
          .AddValue("bool/foo", ResourceUtils::TryParseBool("true"))
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, table_a.get(), false /*overlay*/));
  ASSERT_TRUE(merger.Merge({}, table_b.get(), false /*overlay*/));
}

TEST_F(TableMergerTest, MergeAddResourceFromOverlayWithAutoAddOverlay) {
  std::unique_ptr<ResourceTable> table_a =
      test::ResourceTableBuilder().SetPackageId("", 0x7f).Build();
  std::unique_ptr<ResourceTable> table_b =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .AddValue("bool/foo", ResourceUtils::TryParseBool("true"))
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = true;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, table_a.get(), false /*overlay*/));
  ASSERT_TRUE(merger.Merge({}, table_b.get(), false /*overlay*/));
}

TEST_F(TableMergerTest, FailToMergeNewResourceWithoutAutoAddOverlay) {
  std::unique_ptr<ResourceTable> table_a =
      test::ResourceTableBuilder().SetPackageId("", 0x7f).Build();
  std::unique_ptr<ResourceTable> table_b =
      test::ResourceTableBuilder()
          .SetPackageId("", 0x7f)
          .AddValue("bool/foo", ResourceUtils::TryParseBool("true"))
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = false;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, table_a.get(), false /*overlay*/));
  ASSERT_FALSE(merger.Merge({}, table_b.get(), true /*overlay*/));
}

TEST_F(TableMergerTest, OverlaidStyleablesAndStylesShouldBeMerged) {
  std::unique_ptr<ResourceTable> table_a =
      test::ResourceTableBuilder()
          .SetPackageId("com.app.a", 0x7f)
          .AddValue("com.app.a:styleable/Foo",
                    test::StyleableBuilder()
                        .AddItem("com.app.a:attr/bar")
                        .AddItem("com.app.a:attr/foo", ResourceId(0x01010000))
                        .Build())
          .AddValue("com.app.a:style/Theme",
                    test::StyleBuilder()
                        .SetParent("com.app.a:style/Parent")
                        .AddItem("com.app.a:attr/bar", util::make_unique<Id>())
                        .AddItem("com.app.a:attr/foo", ResourceUtils::MakeBool(false))
                        .Build())
          .Build();

  std::unique_ptr<ResourceTable> table_b =
      test::ResourceTableBuilder()
          .SetPackageId("com.app.a", 0x7f)
          .AddValue("com.app.a:styleable/Foo", test::StyleableBuilder()
                                                   .AddItem("com.app.a:attr/bat")
                                                   .AddItem("com.app.a:attr/foo")
                                                   .Build())
          .AddValue("com.app.a:style/Theme",
                    test::StyleBuilder()
                        .SetParent("com.app.a:style/OverlayParent")
                        .AddItem("com.app.a:attr/bat", util::make_unique<Id>())
                        .AddItem("com.app.a:attr/foo", ResourceId(0x01010000),
                                 ResourceUtils::MakeBool(true))
                        .Build())
          .Build();

  ResourceTable final_table;
  TableMergerOptions options;
  options.auto_add_overlay = true;
  TableMerger merger(context_.get(), &final_table, options);

  ASSERT_TRUE(merger.Merge({}, table_a.get(), false /*overlay*/));
  ASSERT_TRUE(merger.Merge({}, table_b.get(), true /*overlay*/));

  Styleable* styleable = test::GetValue<Styleable>(&final_table, "com.app.a:styleable/Foo");
  ASSERT_THAT(styleable, NotNull());

  std::vector<Reference> expected_refs = {
      Reference(test::ParseNameOrDie("com.app.a:attr/bar")),
      Reference(test::ParseNameOrDie("com.app.a:attr/bat")),
      Reference(test::ParseNameOrDie("com.app.a:attr/foo"), ResourceId(0x01010000)),
  };
  EXPECT_THAT(styleable->entries, UnorderedElementsAreArray(expected_refs));

  Style* style = test::GetValue<Style>(&final_table, "com.app.a:style/Theme");
  ASSERT_THAT(style, NotNull());

  std::vector<Reference> extracted_refs;
  for (const auto& entry : style->entries) {
    extracted_refs.push_back(entry.key);
  }
  EXPECT_THAT(extracted_refs, UnorderedElementsAreArray(expected_refs));

  const auto expected = ResourceUtils::MakeBool(true);
  EXPECT_THAT(style->entries, Contains(Field(&Style::Entry::value, Pointee(ValueEq(*expected)))));
  EXPECT_THAT(style->parent,
              Eq(make_value(Reference(test::ParseNameOrDie("com.app.a:style/OverlayParent")))));
}

}  // namespace aapt
