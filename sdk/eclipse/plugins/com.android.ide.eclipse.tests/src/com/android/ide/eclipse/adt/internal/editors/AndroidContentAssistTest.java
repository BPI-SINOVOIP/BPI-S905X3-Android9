/*

 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ide.eclipse.adt.internal.editors;

import static com.android.SdkConstants.FD_RES;
import static com.android.SdkConstants.FD_RES_ANIM;
import static com.android.SdkConstants.FD_RES_ANIMATOR;
import static com.android.SdkConstants.FD_RES_COLOR;
import static com.android.SdkConstants.FD_RES_DRAWABLE;

import com.android.ide.eclipse.adt.AdtPlugin;
import com.android.ide.eclipse.adt.internal.editors.animator.AnimationContentAssist;
import com.android.ide.eclipse.adt.internal.editors.color.ColorContentAssist;
import com.android.ide.eclipse.adt.internal.editors.common.CommonXmlEditor;
import com.android.ide.eclipse.adt.internal.editors.descriptors.AttributeDescriptor;
import com.android.ide.eclipse.adt.internal.editors.descriptors.ElementDescriptor;
import com.android.ide.eclipse.adt.internal.editors.drawable.DrawableContentAssist;
import com.android.ide.eclipse.adt.internal.editors.layout.LayoutContentAssist;
import com.android.ide.eclipse.adt.internal.editors.layout.descriptors.ViewElementDescriptor;
import com.android.ide.eclipse.adt.internal.editors.layout.refactoring.AdtProjectTest;
import com.android.ide.eclipse.adt.internal.editors.manifest.ManifestContentAssist;
import com.android.ide.eclipse.adt.internal.editors.manifest.ManifestEditor;
import com.android.ide.eclipse.adt.internal.editors.uimodel.UiElementNode;
import com.android.ide.eclipse.adt.internal.editors.values.ValuesContentAssist;

import org.eclipse.core.resources.IFile;
import org.eclipse.jface.text.Document;
import org.eclipse.jface.text.IDocument;
import org.eclipse.jface.text.contentassist.ICompletionProposal;
import org.eclipse.jface.text.source.ISourceViewer;
import org.eclipse.swt.graphics.Point;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.ide.IDE;

@SuppressWarnings("javadoc")
public class AndroidContentAssistTest extends AdtProjectTest {
    private static final String CARET = "^"; //$NON-NLS-1$

    @Override
    protected boolean testCaseNeedsUniqueProject() {
        return true;
    }

    public void testStartsWith() {
        assertTrue(AndroidContentAssist.startsWith("", ""));
        assertTrue(AndroidContentAssist.startsWith("a", ""));
        assertTrue(AndroidContentAssist.startsWith("A", ""));
        assertTrue(AndroidContentAssist.startsWith("A", "a"));
        assertTrue(AndroidContentAssist.startsWith("A", "A"));
        assertTrue(AndroidContentAssist.startsWith("Ab", "a"));
        assertTrue(AndroidContentAssist.startsWith("ab", "A"));
        assertTrue(AndroidContentAssist.startsWith("ab", "AB"));
        assertFalse(AndroidContentAssist.startsWith("ab", "ABc"));
        assertFalse(AndroidContentAssist.startsWith("", "ABc"));
    }

    public void testNameStartsWith() {
        String fullWord = "android:marginTop";
        for (int i = 0; i < fullWord.length(); i++) {
            assertTrue(i + ":" + fullWord.substring(0, i),
                    AndroidContentAssist.nameStartsWith(
                    "android:layout_marginTop", fullWord.substring(0, i), "android:"));
        }

        fullWord = "android:layout_marginTop";
        for (int i = 0; i < fullWord.length(); i++) {
            assertTrue(i + ":" + fullWord.substring(0, i),
                    AndroidContentAssist.nameStartsWith("android:layout_marginTop", fullWord
                    .substring(0, i), "android:"));
        }

        fullWord = "layout_marginTop";
        for (int i = 0; i < fullWord.length(); i++) {
            assertTrue(i + ":" + fullWord.substring(0, i),
                    AndroidContentAssist.nameStartsWith("android:layout_marginTop", fullWord
                    .substring(0, i), "android:"));
        }

        fullWord = "marginTop";
        for (int i = 0; i < fullWord.length(); i++) {
            assertTrue(i + ":" + fullWord.substring(0, i),
                    AndroidContentAssist.nameStartsWith("android:layout_marginTop", fullWord
                    .substring(0, i), "android:"));
        }

        assertFalse(AndroidContentAssist.nameStartsWith("ab", "ABc", null));
        assertFalse(AndroidContentAssist.nameStartsWith("", "ABc", null));
    }

    public void testCompletion1() throws Exception {
        // Change attribute name completion
        checkLayoutCompletion("completion1.xml", "layout_w^idth=\"fill_parent\"");
    }

    public void testCompletion2() throws Exception {
        // Check attribute value completion for enum
        checkLayoutCompletion("completion1.xml", "layout_width=\"^fill_parent\"");
    }

    public void testCompletion3() throws Exception {
        // Check attribute value completion for enum with a prefix
        checkLayoutCompletion("completion1.xml", "layout_width=\"fi^ll_parent\"");
    }

    public void testCompletion4() throws Exception {
        // Check attribute value completion on units
        checkLayoutCompletion("completion1.xml", "marginBottom=\"50^\"");
    }

    public void testCompletion5() throws Exception {
        // Check attribute value completion on units with prefix
        checkLayoutCompletion("completion1.xml", "layout_marginLeft=\"50d^p\"");
    }

    public void testCompletion6() throws Exception {
        // Check resource sorting - "style" should bubble to the top for a style attribute
        checkLayoutCompletion("completion1.xml", "style=\"@android:^style/Widget.Button\"");
    }

    public void testCompletion7a() throws Exception {
        // Check flags (multiple values inside a single XML value, separated by | - where
        // the prefix is reset as soon as you pass each | )
        checkLayoutCompletion("completion1.xml", "android:gravity=\"l^eft|bottom\"");
    }

    public void testCompletion7b() throws Exception {
        checkLayoutCompletion("completion1.xml", "android:gravity=\"left|b^ottom\"");
    }

    public void testCompletion8() throws Exception {
        // Test completion right at the "=" sign; this will be taken to be the last
        // character of the attribute name (the caret is between the last char and before
        // the = characters), so it should match a single attribute
        checkLayoutCompletion("completion1.xml", "layout_width^=\"fill_parent\"");
    }

    public void testCompletion9() throws Exception {
        // Test completion right after the "=" sign; this will be taken to be the beginning
        // of the attribute value, but all values will also include a leading quote
        checkLayoutCompletion("completion1.xml", "layout_width=^\"fill_parent\"");
    }

    public void testCompletion10() throws Exception {
        // Test completion of element names
        checkLayoutCompletion("completion1.xml", "<T^extView");
    }

    public void testCompletion11() throws Exception {
        // Test completion of element names at the outside of the <. This should include
        // all the elements too (along with the leading <).
        checkLayoutCompletion("completion1.xml", "^<TextView");
    }

    public void testCompletion12() throws Exception {
        // Test completion of element names inside a nested XML; ensure that this will
        // correctly compute element names, not previous attribute
        checkLayoutCompletion("completion1.xml", "btn_default\">^</FrameLayout>");
    }

    public void testCompletion13a() throws Exception {
        checkLayoutCompletion("completion2.xml", "gravity=\"left|bottom|^cen");
    }

    public void testCompletion13b() throws Exception {
        checkLayoutCompletion("completion2.xml", "gravity=\"left|bottom|cen^");
    }

    public void testCompletion13c() throws Exception {
        checkLayoutCompletion("completion2.xml", "gravity=\"left|bottom^|cen");
    }

    public void testCompletion14() throws Exception {
        // Test completion of permissions
        checkManifestCompletion("manifest.xml", "android.permission.ACC^ESS_NETWORK_STATE");
    }

    public void testCompletion15() throws Exception {
        // Test completion of intents
        checkManifestCompletion("manifest.xml", "android.intent.category.L^AUNCHER");
    }

    public void testCompletion16() throws Exception {
        // Test completion of top level elements
        checkManifestCompletion("manifest.xml", "<^application android:i");
    }

    public void testCompletion17() throws Exception {
        // Test completion of attributes on the manifest element
        checkManifestCompletion("manifest.xml", "^android:versionCode=\"1\"");
    }

    public void testCompletion18() throws Exception {
        // Test completion of attributes on the manifest element
        checkManifestCompletion("manifest.xml",
                "<activity android:^name=\".TestActivity\"");
    }

    public void testCompletion19() throws Exception {
        // Test special case where completing on a new element in an otherwise blank line
        // does not add in full completion (with closing tags)
        checkLayoutCompletion("broken3.xml", "<EditT^");
    }

    public void testCompletion20() throws Exception {
        checkLayoutCompletion("broken1.xml", "android:textColorHigh^");
    }

    public void testCompletion21() throws Exception {
        checkLayoutCompletion("broken2.xml", "style=^");
    }

    public void testCompletion22() throws Exception {
        // Test completion where the cursor is inside an element (e.g. the next
        // char is NOT a <) - should not complete with end tags
        checkLayoutCompletion("completion4.xml", "<Button^");
    }

    // Test completion in style files

    public void testCompletion23() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "android:textS^ize");
    }

    public void testCompletion24() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "17^sp");
    }

    public void testCompletion25() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "textColor\">^@color/title_color</item>");
    }

    public void testCompletion26() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:shadowColor\">@an^</item>");
    }

    public void testCompletion27() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:gravity\">^  </item>");
    }

    public void testCompletion28() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:gravity\">  ^</item>");
    }

    public void testCompletion29() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "<item name=\"gr^\">");
    }

    public void testCompletion30() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "<item name=\"an^\">");
    }

    public void testCompletion31() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "<item ^></item>");
    }

    public void testCompletion32() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "<item name=\"^\"></item>");
    }

    public void testCompletion33() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:allowSingleTap\">^</item>");
    }

    public void testCompletion34() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:alwaysDrawnWithCache\">^  false  </item>");
    }

    public void testCompletion35() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:alwaysDrawnWithCache\">  ^false  </item>");
    }

    public void testCompletion36() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:alwaysDrawnWithCache\">  f^alse  </item>");
    }

    public void testCompletion37() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "<item name=\"android:orientation\">h^</item>");
    }

    public void testCompletion38() throws Exception {
        checkResourceCompletion("completionvalues1.xml",
                "           c^");
    }

    public void testCompletion39() throws Exception {
        // If you are at the end of a closing quote (but with no space), completion should
        // include a separating space.
        checkLayoutCompletion("completion1.xml", "marginBottom=\"50\"^");
    }

    public void testCompletion40() throws Exception {
        // Same as test 39 but with single quote
        checkLayoutCompletion("completion5.xml",  "android:id='@+id/button2'^");
    }

    public void testCompletion41() throws Exception {
        // Test prefix matching on layout_ with namespace prefix
        checkLayoutCompletion("completion8.xml",  "android:mar^=\"50dp\"");
    }

    public void testCompletion42() throws Exception {
        // Test prefix matching on layout_ with namespace prefix
        checkLayoutCompletion("completion8.xml",  "android:w^i=\"100\"");
    }

    public void testCompletion43() throws Exception {
        // Test prefix matching on layout_ without namespace prefix
        checkLayoutCompletion("completion8.xml",  "mar^=\"60dp\"");
    }

    public void testCompletion44() throws Exception {
        // Test prefix matching on layout_ without namespace prefix
        checkLayoutCompletion("completion8.xml",  "android:layo^ut_width=\"fill_parent\"");
    }

    public void testCompletion45() throws Exception {
        // Test top level elements in colors
        checkColorCompletion("color1.xml",  "^<selector");
    }

    public void testCompletion46a() throws Exception {
        // Test children of selector: should offer item
        checkColorCompletion("color1.xml",  "^<item android");
    }

    public void testCompletion46b() throws Exception {
        // Test attribute matching in color files
        checkColorCompletion("color1.xml",  "<item ^android:state_focused=\"true\"/>");
    }

    public void testCompletion47() throws Exception {
        // Check root completion in drawables: should list all drawable root elements
        checkDrawableCompletion("drawable1.xml",  "^<layer-list");
    }

    public void testCompletion48() throws Exception {
        // Check attributes of the layer list
        checkDrawableCompletion("drawable1.xml",  "^xmlns:android");
    }

    public void testCompletion49() throws Exception {
        // Check attributes of the <item> element inside a <layer-list>
        checkDrawableCompletion("drawable1.xml",  "<item ^></item>");
    }

    public void testCompletion50() throws Exception {
        // Check elements nested inside the <item> in a layer list: can use any drawable again
        checkDrawableCompletion("drawable1.xml",  "<item >^</item>");
    }

    public void testCompletion51() throws Exception {
        // Check attributes of <shape> element
        checkDrawableCompletion("drawable2.xml",  "^android:innerRadiusRatio=\"2\"");
    }

    public void testCompletion52() throws Exception {
        // Check list of available elements inside a shape
        checkDrawableCompletion("drawable2.xml",  "^<gradient");
    }

    public void testCompletion53() throws Exception {
        // Check list of root anim elements
        checkAnimCompletion("anim1.xml",  "^<set xmlns");
    }

    public void testCompletion54() throws Exception {
        // Check that we can nest inside <set>'s
        checkAnimCompletion("anim1.xml",  "^<translate android:id=");
    }

    public void testCompletion55() throws Exception {
        // translate properties
        checkAnimCompletion("anim1.xml",  "android:^fromXDelta=");
    }

    public void testCompletion56() throws Exception {
        // alpha properties
        checkAnimCompletion("anim1.xml",  "android:^fromAlpha=");
    }

    public void testCompletion57() throws Exception {
        // Fractional properties
        checkAnimCompletion("anim1.xml",  "android:fromXDelta=\"100^%p\"");
    }

    public void testCompletion58() throws Exception {
        // Top level animator elements
        checkAnimatorCompletion("animator1.xml",  "^<set xmlns");
    }

    public void testCompletion59() throws Exception {
        // objectAnimator properties
        checkAnimatorCompletion("animator1.xml",  "android:^duration=\"2000\"");
    }

    public void testCompletion60() throws Exception {
        // propertyName completion
        checkAnimatorCompletion("animator1.xml",  "android:propertyName=\"scal^eX\"/>");
    }

    public void testCompletion61() throws Exception {
        // Interpolator completion
        checkAnimatorCompletion("animator1.xml",
                "android:interpolator=\"^@android:anim/bounce_interpolator\"");
    }

    public void testCompletion62() throws Exception {
        // Test completing inside an element that contains .'s, such as a custom view tag
        checkLayoutCompletion("completion9.xml",  "android:layout_wi^dth=");
    }

    public void testCompletion63() throws Exception {
        // Test attribute completion inside a custom view tag
        // TODO: This works in a running IDE but not in a test context; figure out why.
        //checkLayoutCompletion("completion9.xml",  "android:drawable^Top");
    }

    public void testCompletion64() throws Exception {
        // Test element completion inside a custom view tag
        checkLayoutCompletion("completion9.xml",  "^<Button");
    }

    public void testCompletion65() throws Exception {
        // Test completion replacement when there is a selection
        // (see issue http://code.google.com/p/android/issues/detail?id=18607 )
        checkLayoutCompletion("completion10.xml", "\"[^wrap_content]\"");
    }

    public void testCompletion66() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "17[^sp]");
    }

    public void testCompletion67() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "17[^sp]");
    }

    public void testCompletion68() throws Exception {
        checkResourceCompletion("completionvalues1.xml", "[^false]");
    }

    public void testCompletion69() throws Exception {
        // Test minimum SDK completion
        checkManifestCompletion("manifest.xml", "<uses-sdk android:minSdkVersion=\"^11\" />");
    }

    public void testCompletion70() throws Exception {
        checkResourceCompletion("completionvalues2.xml",
                "<item name=\"main_layout4\" type=\"layout\">^</item>");
    }

    public void testCompletion71() throws Exception {
        checkResourceCompletion("completionvalues2.xml",
                "<item name=\"main_layout5\" type=\"string\">@string/^app_name</item>");
    }

    public void testCompletion72() throws Exception {
        // Test completion of theme attributes
        checkLayoutCompletion("completion11.xml", "?^android:attr/Textapp");
    }

    public void testCompletion73() throws Exception {
        checkLayoutCompletion("completion11.xml", "?android:attr/Textapp^");
    }

    public void testCompletion74() throws Exception {
        checkLayoutCompletion("completion11.xml", "?and^roid:attr/Textapp");
    }

    public void testCompletion75() throws Exception {
        // Test <include> attributes
        checkLayoutCompletion("completion12.xml", "<include ^/>");
    }

    public void testComplation76() throws Exception {
        // Test theme completion with implicit attr
        checkLayoutCompletion("navigation1.xml", "?android:a^ttr/alertDialogStyle");
    }

    public void testComplation77() throws Exception {
        // Test <fragment class="^" completion
        checkLayoutCompletion("fragmentlayout.xml", "android:name=\"^com");
    }

    public void testComplation78() throws Exception {
        // Test <fragment android:name="^" completion
        checkLayoutCompletion("fragmentlayout.xml", "class=\"^com");
    }

    public void testComplation79() throws Exception {
        // Test tools context completion
        checkLayoutCompletion("completion11.xml", "tools:context=\"^.MainActivity\"");
    }

    public void testComplation80() throws Exception {
        // Test manifest class completion
        checkManifestCompletion("manifest.xml", "<activity android:name=\"^.");
    }

    // TODO: Test <view completion!

    // ---- Test *applying* code completion ----

    // The following tests check -applying- a specific code completion
    // match - this verifies that the document is updated correctly, the
    // caret is moved appropriately, etc.

    public void testApplyCompletion1() throws Exception {
        // Change attribute name completion
        checkApplyLayoutCompletion("completion1.xml", "layout_w^idth=\"fill_parent\"",
                "android:layout_weight");
    }

    public void testApplyCompletion2() throws Exception {
        // Check attribute value completion for enum
        checkApplyLayoutCompletion("completion1.xml", "layout_width=\"^fill_parent\"",
                "match_parent");
    }

    public void testApplyCompletion3() throws Exception {
        // Check attribute value completion for enum with a prefix
        checkApplyLayoutCompletion("completion1.xml", "layout_width=\"fi^ll_parent\"",
                "fill_parent");
    }

    public void testApplyCompletion4() throws Exception {
        // Check attribute value completion on units
        checkApplyLayoutCompletion("completion1.xml", "marginBottom=\"50^\"", "50mm");
    }

    public void testApplyCompletion5() throws Exception {
        // Check attribute value completion on units with prefix
        checkApplyLayoutCompletion("completion1.xml", "layout_marginLeft=\"50d^p\"", "50dp");
    }

    public void testApplyCompletion6() throws Exception {
        // Check resource sorting - "style" should bubble to the top for a style attribute
        checkApplyLayoutCompletion("completion1.xml", "style=\"@android:^style/Widget.Button\"",
                "@android:drawable/");
    }

    public void testApplyCompletion7a() throws Exception {
        // Check flags (multiple values inside a single XML value, separated by | - where
        // the prefix is reset as soon as you pass each | )
        checkApplyLayoutCompletion("completion1.xml", "android:gravity=\"l^eft|bottom\"",
                "left");
        // NOTE - this will replace all flag values with the newly selected value.
        // That may not be the best behavior - perhaps we should only replace one portion
        // of the value.
    }

    public void testApplyCompletion7b() throws Exception {
        checkApplyLayoutCompletion("completion1.xml", "android:gravity=\"left|b^ottom\"",
                "bottom");
        // NOTE - this will replace all flag values with the newly selected value.
        // That may not be the best behavior - perhaps we should only replace one portion
        // of the value.
    }

    public void testApplyCompletion8() throws Exception {
        // Test completion right at the "=" sign; this will be taken to be the last
        // character of the attribute name (the caret is between the last char and before
        // the = characters), so it should match a single attribute
        checkApplyLayoutCompletion("completion1.xml", "layout_width^=\"fill_parent\"",
                "android:layout_width");
    }

    public void testApplyCompletion9() throws Exception {
        // Test completion right after the "=" sign; this will be taken to be the beginning
        // of the attribute value, but all values will also include a leading quote
        checkApplyLayoutCompletion("completion1.xml", "layout_width=^\"fill_parent\"",
                "\"wrap_content\"");
    }

    public void testApplyCompletion10() throws Exception {
        // Test completion of element names
        checkApplyLayoutCompletion("completion1.xml", "<T^extView", "TableLayout");
    }

    public void testApplyCompletion11a() throws Exception {
        // Test completion of element names at the outside of the <. This should include
        // all the elements too (along with the leading <).
        checkApplyLayoutCompletion("completion1.xml", "^<TextView", "<RadioGroup ></RadioGroup>");
    }

    public void testApplyCompletion11b() throws Exception {
        // Similar to testApplyCompletion11a, but replacing with an element that does not
        // have children (to test the closing tag insertion code)
        checkApplyLayoutCompletion("completion1.xml", "^<TextView", "<CheckBox />");
    }

    public void testApplyCompletion12() throws Exception {
        // Test completion of element names inside a nested XML; ensure that this will
        // correctly compute element names, not previous attribute
        checkApplyLayoutCompletion("completion1.xml", "btn_default\">^</FrameLayout>",
                "<FrameLayout ></FrameLayout>");
    }

    public void testApplyCompletion13a() throws Exception {
        checkApplyLayoutCompletion("completion2.xml", "gravity=\"left|bottom|^cen",
                "fill_vertical");
    }

    public void testApplyCompletion13b() throws Exception {
        checkApplyLayoutCompletion("completion2.xml", "gravity=\"left|bottom|cen^",
                "center_horizontal");
    }

    public void testApplyCompletion13c() throws Exception {
        checkApplyLayoutCompletion("completion2.xml", "gravity=\"left|bottom^|cen",
                "bottom|fill_horizontal");
    }

    public void testApplyCompletion14() throws Exception {
        // Test special case where completing on a new element in an otherwise blank line
        // does not add in full completion (with closing tags)
        checkApplyLayoutCompletion("broken3.xml", "<EditT^", "EditText />");
    }

    public void testApplyCompletion15() throws Exception {
        checkApplyLayoutCompletion("broken1.xml", "android:textColorHigh^",
                "android:textColorHighlight");
    }

    public void testApplyCompletion16() throws Exception {
        checkApplyLayoutCompletion("broken2.xml", "style=^",
                "\"@android:\"");
    }

    public void testApplyCompletion17() throws Exception {
        // Make sure that completion right before a / inside an element still
        // inserts the ="" part (e.g. handles it as "insertNew)
        checkApplyLayoutCompletion("completion3.xml", "<EditText ^/>",
                "android:textColorHighlight");
    }

    public void testApplyCompletion18() throws Exception {
        // Make sure that completion right before a > inside an element still
        // inserts the ="" part (e.g. handles it as "insertNew)
        checkApplyLayoutCompletion("completion3.xml", "<Button ^></Button>",
                "android:paddingRight");
    }

    public void testApplyCompletion19() throws Exception {
        // Test completion with single quotes (apostrophe)
        checkApplyLayoutCompletion("completion5.xml", "android:orientation='^'", "horizontal");
    }

    public void testApplyCompletion20() throws Exception {
        // Test completion with single quotes (apostrophe)
        checkApplyLayoutCompletion("completion5.xml", "android:layout_marginTop='50^dp'", "50pt");
    }

    public void testApplyCompletion21() throws Exception {
        // Test completion with single quotes (apostrophe)
        checkApplyLayoutCompletion("completion5.xml", "android:layout_width='^wrap_content'",
                "match_parent");
        // Still broken - but not a common case
        //checkApplyLayoutCompletion("completion5.xml", "android:layout_width=^'wrap_content'",
        //     "\"match_parent\"");
    }

    public void testApplyCompletion22() throws Exception {
        // Test completion in an empty string
        checkApplyLayoutCompletion("completion6.xml", "android:orientation=\"^\"", "horizontal");
    }

    public void testApplyCompletion23() throws Exception {
        // Test completion in an empty string
        checkApplyLayoutCompletion("completion7.xml", "android:orientation=\"^", "horizontal");
    }

    // Test completion in style files

    public void testApplyCompletion24a() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "android:textS^ize",
                "android:textSelectHandleLeft");
    }

    public void testApplyCompletion24b() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "17^sp", "17mm");
    }

    public void testApplyCompletion25() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "textColor\">^@color/title_color</item>", "@android:");
    }

    public void testApplyCompletion26() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:shadowColor\">@an^</item>", "@android:");
    }

    public void testApplyCompletion27() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:gravity\">^  </item>", "center_vertical");
    }

    public void testApplyCompletion28() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:gravity\">  ^</item>", "left");
    }

    public void testApplyCompletion29() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "<item name=\"gr^\">",
                "android:gravity");
    }

    public void testApplyCompletion30() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "<item name=\"an^\">",
                "android:animateOnClick");
    }

    public void testApplyCompletion31() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "<item ^></item>", "name");
    }

    public void testApplyCompletion32() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "<item name=\"^\"></item>",
                "android:background");
    }

    public void testApplyCompletion33() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:allowSingleTap\">^</item>", "true");
    }

    public void testApplyCompletion34() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:alwaysDrawnWithCache\">^  false  </item>", "true");
    }

    public void testApplyCompletion35() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:alwaysDrawnWithCache\">  ^false  </item>", "true");
    }

    public void testApplyCompletion36() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:alwaysDrawnWithCache\">  f^alse  </item>", "false");
    }

    public void testApplyCompletion37() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "<item name=\"android:orientation\">h^</item>", "horizontal");
    }

    public void testApplyCompletion38() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml",
                "           c^", "center");
    }

    public void testApplyCompletion39() throws Exception {
        // If you are at the end of a closing quote (but with no space), completion should
        // include a separating space.
        checkApplyLayoutCompletion("completion1.xml", "marginBottom=\"50\"^", " android:maxEms");
    }

    public void testApplyCompletion40() throws Exception {
        // If you are at the end of a closing quote (but with no space), completion should
        // include a separating space.
        checkApplyLayoutCompletion("completion5.xml",  "android:id='@+id/button2'^",
                " android:maxWidth");
    }

    public void testApplyCompletion41() throws Exception {
        // Test prefix matching on layout_ with namespace prefix
        checkApplyLayoutCompletion("completion8.xml",  "android:mar^=\"50dp\"",
                "android:layout_marginRight");
    }

    public void testApplyCompletion42() throws Exception {
        // Test completion replacement when there is a selection
        // (see issue http://code.google.com/p/android/issues/detail?id=18607 )
        checkApplyLayoutCompletion("completion10.xml", "\"[^wrap_content]\"", "fill_parent");
    }

    public void testApplyCompletion43() throws Exception {
        // Same as testApplyCompletion42 but with a smaller selection range
        checkApplyLayoutCompletion("completion10.xml", "\"[^wrap_c]ontent\"", "fill_parent");
    }

    public void testApplyCompletion44() throws Exception {
        checkApplyResourceCompletion("completionvalues1.xml", "[^false]", "true");
    }

    public void testApplyCompletion45() throws Exception {
        checkApplyResourceCompletion("completionvalues2.xml",
                "@string/^app_name", "@string/hello");
    }

    public void testApplyCompletion46() throws Exception {
        checkApplyLayoutCompletion("completion11.xml",
                "?android:attr/Textapp^", "?android:attr/textAppearanceLargeInverse");
    }

    public void testApplyCompletion47() throws Exception {
        // Test applying <fragment android:name="^" completion
        checkApplyLayoutCompletion("fragmentlayout.xml", "class=\"^com",
                "android.app.ListFragment");
    }

    // --- Code Completion test infrastructure ----

    private void checkLayoutCompletion(String name, String caretLocation) throws Exception {
        IFile file = getLayoutFile(getProject(), name);
        IDE.setDefaultEditor(file, CommonXmlEditor.ID);
        checkCompletion(name, file, caretLocation,
                new LayoutContentAssist());
    }

    private void checkColorCompletion(String name, String caretLocation) throws Exception {
        IFile file = getTestDataFile(getProject(), name, FD_RES + "/" + FD_RES_COLOR + "/" + name);
        IDE.setDefaultEditor(file, CommonXmlEditor.ID);
        checkCompletion(name, file, caretLocation, new ColorContentAssist());
    }

    private void checkAnimCompletion(String name, String caretLocation) throws Exception {
        IFile file = getTestDataFile(getProject(), name, FD_RES + "/" + FD_RES_ANIM + "/" + name);
        IDE.setDefaultEditor(file, CommonXmlEditor.ID);
        checkCompletion(name, file, caretLocation, new AnimationContentAssist());
    }

    private void checkAnimatorCompletion(String name, String caretLocation) throws Exception {
        IFile file = getTestDataFile(getProject(), name, FD_RES + "/" + FD_RES_ANIMATOR + "/"
                + name);
        IDE.setDefaultEditor(file, CommonXmlEditor.ID);
        checkCompletion(name, file, caretLocation, new AnimationContentAssist());
    }

    private void checkDrawableCompletion(String name, String caretLocation) throws Exception {
        IFile file = getTestDataFile(getProject(), name, FD_RES + "/" + FD_RES_DRAWABLE + "/"
                + name);
        IDE.setDefaultEditor(file, CommonXmlEditor.ID);
        checkCompletion(name, file, caretLocation, new DrawableContentAssist());
    }

    private void checkManifestCompletion(String name, String caretLocation) throws Exception {
        // Manifest files must be named AndroidManifest.xml. Must overwrite to replace
        // the default manifest created in the test project.
        IFile file = getTestDataFile(getProject(), name, "AndroidManifest.xml", true);
        IDE.setDefaultEditor(file, ManifestEditor.ID);

        checkCompletion(name, file, caretLocation,
                new ManifestContentAssist());
    }

    private void checkApplyLayoutCompletion(String name, String caretLocation,
            String match) throws Exception {
        checkApplyCompletion(name, getLayoutFile(getProject(), name), caretLocation,
                new LayoutContentAssist(), match);
    }

    private void checkResourceCompletion(String name, String caretLocation) throws Exception {
        checkCompletion(name, getValueFile(getProject(), name), caretLocation,
                new ValuesContentAssist());
    }

    private void checkApplyResourceCompletion(String name, String caretLocation,
            String match) throws Exception {
        checkApplyCompletion(name, getValueFile(getProject(), name), caretLocation,
                new ValuesContentAssist(), match);
    }

    private ICompletionProposal[] complete(IFile file, String caretLocation,
            AndroidContentAssist assist) throws Exception {

        // Open file
        IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
        assertNotNull(page);
        IEditorPart editor = IDE.openEditor(page, file);
        assertTrue(editor instanceof AndroidXmlEditor);
        AndroidXmlEditor xmlEditor = (AndroidXmlEditor) editor;

        UiElementNode root = xmlEditor.getUiRootNode();
        ElementDescriptor descriptor = root.getDescriptor();
        if (descriptor instanceof ViewElementDescriptor) {
            ViewElementDescriptor vd = (ViewElementDescriptor) descriptor;
            AttributeDescriptor[] attributes = vd.getAttributes();
            assertTrue(Integer.toString(attributes.length), attributes.length > 0);
        }

        ISourceViewer viewer = xmlEditor.getStructuredSourceViewer();

        // Determine the offset, and possibly make text range selections as well
        int offset = updateCaret(viewer, caretLocation);

        // Run code completion
        ICompletionProposal[] proposals = assist.computeCompletionProposals(viewer, offset);
        if (proposals == null) {
            proposals = new ICompletionProposal[0];
        }

        editor.getEditorSite().getPage().closeAllEditors(false);

        return proposals;
    }

    private void checkApplyCompletion(String basename, IFile file, String caretLocation,
            AndroidContentAssist assist, String match) throws Exception {
        ICompletionProposal[] proposals = complete(file, caretLocation, assist);
        ICompletionProposal chosen = null;
        for (ICompletionProposal proposal : proposals) {
            if (proposal.getDisplayString().equals(match)) {
                chosen = proposal;
                break;
            }
        }
        assertNotNull(chosen);
        assert chosen != null; // Eclipse null pointer analysis doesn't believe the JUnit assertion

        String fileContent = AdtPlugin.readFile(file);
        IDocument document = new Document();
        document.set(fileContent);

        // Apply code completion
        chosen.apply(document);

        // Insert caret location as well
        Point location = chosen.getSelection(document);
        document.replace(location.x, 0, CARET);

        String actual = document.get();

        int offset = getCaretOffset(fileContent, caretLocation);
        String beforeWithCaret = fileContent.substring(0, offset) + CARET
                + fileContent.substring(offset);

        String diff = getDiff(beforeWithCaret, actual);
        assertTrue(diff + " versus " + actual, diff.length() > 0 || beforeWithCaret.equals(actual));

        StringBuilder summary = new StringBuilder();
        summary.append("Code completion in " + basename + " for " + caretLocation + " selecting "
                + match + ":\n");
        if (diff.length() == 0) {
            diff = "No changes";
        }
        summary.append(diff);

        // assertEqualsGolden(basename, actual);
        assertEqualsGolden(basename, summary.toString(), "diff");
    }

    private void checkCompletion(String basename, IFile file, String caretLocation,
            AndroidContentAssist assist) throws Exception {
        ICompletionProposal[] proposals = complete(file, caretLocation, assist);
        StringBuilder sb = new StringBuilder(1000);
        sb.append("Code completion in " + basename + " for " + caretLocation + ":\n");
        for (ICompletionProposal proposal : proposals) {
            // TODO: assertNotNull(proposal.getImage());
            int length = sb.length();
            sb.append(proposal.getDisplayString().trim());
            String help = proposal.getAdditionalProposalInfo();
            if (help != null && help.trim().length() > 0) {
                sb.append(" : ");
                sb.append(help.replace('\n', ' ').trim());
                if (sb.length() > length + 300) {
                    sb.setLength(length + 300 - "...".length());
                    sb.append("...");
                }
            }
            sb.append('\n');
        }
        assertEqualsGolden(basename, sb.toString(), "txt");
    }
}
