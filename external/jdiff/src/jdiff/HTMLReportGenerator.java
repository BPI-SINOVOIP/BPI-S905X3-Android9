package jdiff;

import java.util.*;
import java.io.*;
import java.text.*;

/**
 * Emit HTML based on the changes between two sets of APIs.
 *
 * See the file LICENSE.txt for copyright details.
 * @author Matthew Doar, mdoar@pobox.com
 */
public class HTMLReportGenerator {

    /** Default constructor. */
    public HTMLReportGenerator() {
    }   

    /** The Comments object for existing comments. */
    private Comments existingComments_ = null;

    /** 
     * The Comments object for freshly regenerated comments. 
     * This is populated during the generation of the report,
     * and should be like existingComments_ but with unused comments
     * marked as such, so that they can be commented out in XML when 
     * the new comments are written out to the comments file.
     */
    private Comments newComments_ = null;

    /** 
     * Accessor method for the freshly generated Comments object. 
     * The list of comments is sorted before the object is returned.
     */
    public Comments getNewComments() {
        Collections.sort(newComments_.commentsList_);
        return newComments_;
    }

    /** Generate the report. */
    public void generate(APIComparator comp, Comments existingComments) {
        String fullReportFileName = reportFileName;
        if (outputDir != null)
            fullReportFileName = outputDir + JDiff.DIR_SEP + reportFileName;
        System.out.println("JDiff: generating HTML report into the file '" + fullReportFileName + reportFileExt + "' and the subdirectory '" + fullReportFileName + "'");
        // May be null if no comments file exists yet
        existingComments_ = existingComments;
        // Where the new comments will be placed
        newComments_ = new Comments();
        // Writing to multiple files, so make sure the subdirectory exists
        File opdir = new File(fullReportFileName);
        if (!opdir.mkdir() && !opdir.exists()) {
            System.out.println("Error: could not create the subdirectory '" + fullReportFileName + "'");
            System.exit(3);
        }

        // Emit the documentation difference files
        if (!Diff.noDocDiffs) {
            // Documentation differences, one file per package
            Diff.emitDocDiffs(fullReportFileName);
        }

        // This is the top-level summary file, first in the right hand frame
        // or linked at the start to if no frames are used.
        String changesSummaryName = fullReportFileName + JDiff.DIR_SEP +
            reportFileName + "-summary" + reportFileExt;
        apiDiff = comp.apiDiff;
        try {
            FileOutputStream fos = new FileOutputStream(changesSummaryName);
            reportFile = new PrintWriter(fos);
            writeStartHTMLHeader();
            // Write out the title in he HTML header
            String oldAPIName = "Old API";
            if (apiDiff.oldAPIName_ != null)
                oldAPIName = apiDiff.oldAPIName_;
            String newAPIName = "New API";
            if (apiDiff.newAPIName_ != null)
                newAPIName = apiDiff.newAPIName_;
            if (windowTitle == null) 
                writeHTMLTitle("Android API Differences Report");
            else
                writeHTMLTitle(windowTitle);
            writeStyleSheetRef();
            writeText("</HEAD>");

            writeText("<body class=\"gc-documentation\">");

           // writeText("<div class=\"g-section g-tpl-180\">");
           // Add the nav bar for the summary page
            writeNavigationBar(reportFileName + "-summary", null, null,
                               null, 0, true,
                               apiDiff.packagesRemoved.size() != 0,
                               apiDiff.packagesAdded.size() != 0,
                               apiDiff.packagesChanged.size() != 0);

            writeText("    <div id=\"docTitleContainer\">");
            // Write the title in the body with some formatting
            if (docTitle == null) {
	                //writeText("<center>");
                writeText("<h1>Android&nbsp;API&nbsp;Differences&nbsp;Report</h1>");
            } else {
                writeText("    <h1>" + docTitle + "</h1>");
            }

            writeText("<p>This report details the changes in the core Android framework API between two <a ");  writeText("href=\"https://developer.android.com/guide/appendix/api-levels.html\" target=\"_top\">API Level</a> ");
            writeText("specifications. It shows additions, modifications, and removals for packages, classes, methods, and fields. ");
            writeText("The report also includes general statistics that characterize the extent and type of the differences.</p>");

            writeText("<p>This report is based a comparison of the Android API specifications ");
            writeText("whose API Level identifiers are given in the upper-right corner of this page. It compares a ");
            writeText("newer \"to\" API to an older \"from\" API, noting all changes relative to the ");
            writeText("older API. So, for example, API elements marked as removed are no longer present in the \"to\" ");
            writeText("API specification.</p>");

            writeText("<p>To navigate the report, use the \"Select a Diffs Index\" and \"Filter the Index\" ");
            writeText("controls on the left. The report uses text formatting to indicate <em>interface names</em>, ");
            writeText("<a href= ><code>links to reference documentation</code></a>, and <a href= >links to change ");
            writeText("description</a>. The statistics are accessible from the \"Statistics\" link in the upper-right corner.</p>");

            writeText("<p>For more information about the Android framework API and SDK, ");
            writeText("see the <a href=\"https://developer.android.com/index.html\" target=\"_top\">Android Developers site</a>.</p>");

            // Write the contents and the other files as well
            writeReport(apiDiff);
            writeHTMLFooter();
            reportFile.close();
        } catch(IOException e) {
            System.out.println("IO Error while attempting to create " + changesSummaryName);
            System.out.println("Error: " + e.getMessage());
            System.exit(1);
        }

        // Now generate all the other files for multiple frames.
        //
        // The top-level changes.html frames file where everything starts.
        String tln = fullReportFileName + reportFileExt;
        // The file for the top-left frame.
        String tlf = fullReportFileName + JDiff.DIR_SEP + 
            "jdiff_topleftframe" + reportFileExt;
        // The default file for the bottom-left frame is the one with the
        // most information in it.
        String allDiffsIndexName = fullReportFileName + JDiff.DIR_SEP + 
            "alldiffs_index"; 
        // Other indexes for the bottom-left frame.
        String packagesIndexName = fullReportFileName + JDiff.DIR_SEP + 
            "packages_index"; 
        String classesIndexName = fullReportFileName + JDiff.DIR_SEP + 
            "classes_index"; 
        String constructorsIndexName = fullReportFileName + JDiff.DIR_SEP + 
            "constructors_index"; 
        String methodsIndexName = fullReportFileName + JDiff.DIR_SEP + 
            "methods_index"; 
        String fieldsIndexName = fullReportFileName + JDiff.DIR_SEP + 
            "fields_index"; 

        HTMLFiles hf = new HTMLFiles(this);
        hf.emitTopLevelFile(tln, apiDiff);
        hf.emitTopLeftFile(tlf);
        hf.emitHelp(fullReportFileName, apiDiff);
        hf.emitStylesheet();

        HTMLIndexes h = new HTMLIndexes(this);
        h.emitAllBottomLeftFiles(packagesIndexName, classesIndexName, 
                            constructorsIndexName, methodsIndexName,
                            fieldsIndexName, allDiffsIndexName, apiDiff);

        if (doStats) {
            // The file for the statistical report.
            String sf = fullReportFileName + JDiff.DIR_SEP + 
                "jdiff_statistics" + reportFileExt;
            HTMLStatistics stats = new HTMLStatistics(this);
            stats.emitStatistics(sf, apiDiff);
        }
    }   

    /** 
     * Write the HTML report. 
     *
     * The top section describes all the packages added (with links) and 
     * removed, and the changed packages section has links which takes you 
     * to a section for each package. This pattern continues for classes and
     * constructors, methods and fields.
     */
    public void writeReport(APIDiff apiDiff) {

        // Report packages which were removed in the new API
        if (!apiDiff.packagesRemoved.isEmpty()) {
            writeTableStart("Removed Packages", 2);
            for (PackageAPI pkgAPI : apiDiff.packagesRemoved) {
                String pkgName = pkgAPI.name_;
                if (trace) System.out.println("Package " + pkgName + " was removed.");
                writePackageTableEntry(pkgName, 0, pkgAPI.doc_, false);
            }
            writeTableEnd();
        }
        
        // Report packages which were added in the new API
        if (!apiDiff.packagesAdded.isEmpty()) {
            writeTableStart("Added Packages", 2);
            for (PackageAPI pkgAPI : apiDiff.packagesAdded) {
                String pkgName = pkgAPI.name_;
                if (trace) System.out.println("Package " + pkgName + " was added.");
                writePackageTableEntry(pkgName, 1, pkgAPI.doc_, false);
            }
            writeTableEnd();

            // Now emit a separate file for each added package.
            for (PackageAPI pkgAPI : apiDiff.packagesAdded) {
                reportAddedPackage(pkgAPI);
            }
        }

        // Report packages which were changed in the new API
        if (!apiDiff.packagesChanged.isEmpty()) {
            // Emit a table of changed packages, with links to the file
            // for each package.
            writeTableStart("Changed Packages", 3);
            for (PackageDiff pkgDiff : apiDiff.packagesChanged) {
                String pkgName = pkgDiff.name_;
                if (trace) System.out.println("Package " + pkgName + " was changed.");
                writePackageTableEntry(pkgName, 2, null, false);
            }
            writeTableEnd();

            // Now emit a separate file for each changed package.
            for (PackageDiff pkgDiff : apiDiff.packagesChanged) {
                reportChangedPackage(pkgDiff);
            }
        }
            writeText("      </div>	");
            writeText("      <div id=\"footer\">");
            writeText("        <div id=\"copyright\">");
            writeText("        Except as noted, this content is licensed under ");
            writeText("        <a href=\"https://creativecommons.org/licenses/by/2.5/\"> Creative Commons Attribution 2.5</a>.");
            writeText("        For details and restrictions, see the <a href=\"https://developer.android.com/license.html\">Content License</a>.");
            writeText("        </div>");
            writeText("      <div id=\"footerlinks\">");
            writeText("      <p>");
            writeText("        <a href=\"https://www.android.com/terms.html\">Site Terms of Service</a> -");
            writeText("        <a href=\"https://www.android.com/privacy.html\">Privacy Policy</a> -");
            writeText("        <a href=\"https://www.android.com/branding.html\">Brand Guidelines</a>");
            writeText("      </p>");
            writeText("    </div>");
            writeText("    </div> <!-- end footer -->");
            writeText("    </div><!-- end doc-content -->");
            writeText("    </div> <!-- end body-content --> ");
    }

    /**
     * Write out a quick redirection file for added packages.
     */
    public void reportAddedPackage(PackageAPI pkgAPI) {
        String pkgName = pkgAPI.name_;

        String localReportFileName = reportFileName + JDiff.DIR_SEP + "pkg_" + pkgName
                + reportFileExt;
        if (outputDir != null)
            localReportFileName = outputDir + JDiff.DIR_SEP + localReportFileName;

        try (PrintWriter pw = new PrintWriter(new FileOutputStream(localReportFileName))) {
            // Link to HTML file for the package
            String pkgRef = newDocPrefix + pkgName.replace('.', '/');
            pw.write("<html><head><meta http-equiv=\"refresh\" content=\"0;URL='" + pkgRef
                    + "/package-summary.html'\" /></head></html>");
        } catch(IOException e) {
            System.out.println("IO Error while attempting to create " + localReportFileName);
            System.out.println("Error: "+ e.getMessage());
            System.exit(1);
        }

        for (ClassAPI classAPI : pkgAPI.classes_) {
            reportAddedClass(pkgAPI.name_, classAPI);
        }
    }

    /**
     * Write out the details of a changed package in a separate file.
     */
    public void reportChangedPackage(PackageDiff pkgDiff) {
        String pkgName = pkgDiff.name_;

        PrintWriter oldReportFile = null;
        oldReportFile = reportFile;
        String localReportFileName = null;
        try {
            // Prefix package files with pkg_ because there may be a class
            // with the same name.
            localReportFileName = reportFileName + JDiff.DIR_SEP + "pkg_" + pkgName + reportFileExt;
            if (outputDir != null)
                localReportFileName = outputDir + JDiff.DIR_SEP + localReportFileName;
            FileOutputStream fos = new FileOutputStream(localReportFileName);
            reportFile = new PrintWriter(fos);
            writeStartHTMLHeader();
            writeHTMLTitle(pkgName);
            writeStyleSheetRef();
            writeText("</HEAD>");
            writeText("<BODY>");
        } catch(IOException e) {
            System.out.println("IO Error while attempting to create " + localReportFileName);
            System.out.println("Error: "+ e.getMessage());
            System.exit(1);
        }

        String pkgRef = pkgName;
        pkgRef = pkgRef.replace('.', '/');
        pkgRef = newDocPrefix + pkgRef + "/package-summary";
        // A link to the package in the new API
        String linkedPkgName = "<A HREF=\"" + pkgRef + ".html\" target=\"_top\"><font size=\"+1\"><code>" + pkgName + "</code></font></A>";
        String prevPkgRef = null;
        String nextPkgRef = null;

        writeSectionHeader("Package " + linkedPkgName, pkgName, 
                           prevPkgRef, nextPkgRef,
                           null, 1,
                           pkgDiff.classesRemoved.size() != 0, 
                           pkgDiff.classesAdded.size() != 0,
                           pkgDiff.classesChanged.size() != 0);

        // Report changes in documentation
        if (reportDocChanges && pkgDiff.documentationChange_ != null) {
            String pkgDocRef = pkgName + "/package-summary";
            pkgDocRef = pkgDocRef.replace('.', '/');
            String oldPkgRef = pkgDocRef;
            String newPkgRef = pkgDocRef;
            if (oldDocPrefix != null)
                oldPkgRef = oldDocPrefix + oldPkgRef;
            else
                oldPkgRef = null;
            newPkgRef = newDocPrefix + newPkgRef;
            if (oldPkgRef != null)
                pkgDiff.documentationChange_ += "<A HREF=\"" + oldPkgRef +
                    ".html#package_description\" target=\"_self\"><code>old</code></A> to ";
            else
                pkgDiff.documentationChange_ += "<font size=\"+1\"><code>old</code></font> to ";
            pkgDiff.documentationChange_ += "<A HREF=\"" + newPkgRef +
                ".html#package_description\" target=\"_self\"><code>new</code></A>. ";
            writeText(pkgDiff.documentationChange_);
        }

        // Report classes which were removed in the new API
        if (pkgDiff.classesRemoved.size() != 0) {
            // Determine the title for this section
            boolean hasClasses = false;
            boolean hasInterfaces = false;
            Iterator iter = pkgDiff.classesRemoved.iterator();
            while (iter.hasNext()) {
                ClassAPI classAPI = (ClassAPI)(iter.next());
                if (classAPI.isInterface_)
                    hasInterfaces = true;
                else
                    hasClasses = true;
            }
            if (hasInterfaces && hasClasses)
                writeTableStart("Removed Classes and Interfaces", 2);
            else if (!hasInterfaces && hasClasses)
                     writeTableStart("Removed Classes", 2);
            else if (hasInterfaces && !hasClasses)
                     writeTableStart("Removed Interfaces", 2);
            // Emit the table entries
            iter = pkgDiff.classesRemoved.iterator();
            while (iter.hasNext()) {
                ClassAPI classAPI = (ClassAPI)(iter.next());
                String className = classAPI.name_;
                if (trace) System.out.println("Class/Interface " + className + " was removed.");
                writeClassTableEntry(pkgName, className, 0, classAPI.isInterface_, classAPI.doc_, false);
            }
            writeTableEnd();
        }
        
        // Report classes which were added in the new API
        if (pkgDiff.classesAdded.size() != 0) {
            // Determine the title for this section
            boolean hasClasses = false;
            boolean hasInterfaces = false;
            Iterator iter = pkgDiff.classesAdded.iterator();
            while (iter.hasNext()) {
                ClassAPI classAPI = (ClassAPI)(iter.next());
                if (classAPI.isInterface_)
                    hasInterfaces = true;
                else
                    hasClasses = true;
            }
            if (hasInterfaces && hasClasses)
                writeTableStart("Added Classes and Interfaces", 2);
            else if (!hasInterfaces && hasClasses)
                     writeTableStart("Added Classes", 2);
            else if (hasInterfaces && !hasClasses)
                     writeTableStart("Added Interfaces", 2);
            // Emit the table entries
            iter = pkgDiff.classesAdded.iterator();
            while (iter.hasNext()) {
                ClassAPI classAPI = (ClassAPI)(iter.next());
                String className = classAPI.name_;
                if (trace) System.out.println("Class/Interface " + className + " was added.");
                writeClassTableEntry(pkgName, className, 1, classAPI.isInterface_, classAPI.doc_, false);
            }
            writeTableEnd();
            // Now emit a separate file for each added class and interface.
            for (ClassAPI classApi : pkgDiff.classesAdded) {
                reportAddedClass(pkgName, classApi);
            }
        }

        // Report classes which were changed in the new API
        if (pkgDiff.classesChanged.size() != 0) {
            // Determine the title for this section
            boolean hasClasses = false;
            boolean hasInterfaces = false;
            Iterator iter = pkgDiff.classesChanged.iterator();
            while (iter.hasNext()) {
                ClassDiff classDiff = (ClassDiff)(iter.next());
                if (classDiff.isInterface_)
                    hasInterfaces = true;
                else
                    hasClasses = true;
            }
            if (hasInterfaces && hasClasses)
                writeTableStart("Changed Classes and Interfaces", 2);
            else if (!hasInterfaces && hasClasses)
                     writeTableStart("Changed Classes", 2);
            else if (hasInterfaces && !hasClasses)
                     writeTableStart("Changed Interfaces", 2);
            // Emit a table of changed classes, with links to the file
            // for each class.
            iter = pkgDiff.classesChanged.iterator();
            while (iter.hasNext()) {
                ClassDiff classDiff = (ClassDiff)(iter.next());
                String className = classDiff.name_;
                if (trace) System.out.println("Package " + pkgDiff.name_ + ", class/Interface " + className + " was changed.");
                writeClassTableEntry(pkgName, className, 2, classDiff.isInterface_, null, false);
            }
            writeTableEnd();
            // Now emit a separate file for each changed class and interface.
            ClassDiff[] classDiffs = new ClassDiff[pkgDiff.classesChanged.size()];
            classDiffs = (ClassDiff[])pkgDiff.classesChanged.toArray(classDiffs);
            for (int k = 0; k < classDiffs.length; k++) {
                reportChangedClass(pkgName, classDiffs, k);
            }
        }
        
        writeSectionFooter(pkgName, prevPkgRef, nextPkgRef, null, 1);
        writeHTMLFooter();
        reportFile.close();
        reportFile = oldReportFile;
    }

    /**
     * Write out a quick redirection file for added classes.
     */
    public void reportAddedClass(String pkgName, ClassAPI classApi) {
        String className = classApi.name_;

        String localReportFileName = reportFileName + JDiff.DIR_SEP + pkgName + "." + className
                + reportFileExt;
        if (outputDir != null)
            localReportFileName = outputDir + JDiff.DIR_SEP + localReportFileName;

        try (PrintWriter pw = new PrintWriter(new FileOutputStream(localReportFileName))) {
            // Link to HTML file for the class
            String classRef = pkgName + "." + className;
            // Deal with inner classes
            if (className.indexOf('.') != -1) {
                classRef = pkgName + ".";
                classRef = classRef.replace('.', '/');
                classRef = newDocPrefix + classRef + className;
            } else {
                classRef = classRef.replace('.', '/');
                classRef = newDocPrefix + classRef;
            }

            pw.write("<html><head><meta http-equiv=\"refresh\" content=\"0;URL='" + classRef
                    + ".html'\" /></head></html>");
        } catch(IOException e) {
            System.out.println("IO Error while attempting to create " + localReportFileName);
            System.out.println("Error: "+ e.getMessage());
            System.exit(1);
        }
    }

    /** 
     * Write out the details of a changed class in a separate file. 
     */
    public void reportChangedClass(String pkgName, ClassDiff[] classDiffs, int classIndex) {
        ClassDiff classDiff = classDiffs[classIndex];
        String className = classDiff.name_;

        PrintWriter oldReportFile = null;
        oldReportFile = reportFile;
        String localReportFileName = null;
        try {
            localReportFileName = reportFileName + JDiff.DIR_SEP + pkgName + "." + className + reportFileExt;
            if (outputDir != null)
                localReportFileName = outputDir + JDiff.DIR_SEP + localReportFileName;
            FileOutputStream fos = new FileOutputStream(localReportFileName);
            reportFile = new PrintWriter(fos);
            writeStartHTMLHeader();
            writeHTMLTitle(pkgName + "." + className);
            writeStyleSheetRef();
            writeText("</HEAD>");
            writeText("<BODY>");
        } catch(IOException e) {
            System.out.println("IO Error while attempting to create " + localReportFileName);
            System.out.println("Error: "+ e.getMessage());
            System.exit(1);
        }

        String classRef = pkgName + "." + className;
        classRef = classRef.replace('.', '/');
        if (className.indexOf('.') != -1) {
            classRef = pkgName + ".";
            classRef = classRef.replace('.', '/');
            classRef = newDocPrefix + classRef + className;
        } else {
            classRef = newDocPrefix + classRef;
        }
        // A link to the class in the new API
        String linkedClassName = "<A HREF=\"" + classRef + ".html\" target=\"_top\"><font size=\"+2\"><code>" + className + "</code></font></A>";
        String lcn = pkgName + "." + linkedClassName;
        //Links to the previous and next classes
        String prevClassRef = null;
        if (classIndex != 0) {
            prevClassRef = pkgName + "." + classDiffs[classIndex-1].name_ + reportFileExt;
        }
        // Create the HTML link to the next package
        String nextClassRef = null;
        if (classIndex < classDiffs.length - 1) {
            nextClassRef = pkgName + "." + classDiffs[classIndex+1].name_ + reportFileExt;
        }

        if (classDiff.isInterface_)
            lcn = "Interface " + lcn;
        else
            lcn = "Class " + lcn;
        boolean hasCtors = classDiff.ctorsRemoved.size() != 0 ||
            classDiff.ctorsAdded.size() != 0 ||
            classDiff.ctorsChanged.size() != 0;
        boolean hasMethods = classDiff.methodsRemoved.size() != 0 ||
            classDiff.methodsAdded.size() != 0 ||
            classDiff.methodsChanged.size() != 0;
        boolean hasFields = classDiff.fieldsRemoved.size() != 0 ||
            classDiff.fieldsAdded.size() != 0 ||
            classDiff.fieldsChanged.size() != 0;
        writeSectionHeader(lcn, pkgName, prevClassRef, nextClassRef, 
                           className, 2,
                           hasCtors, hasMethods, hasFields);

        if (classDiff.inheritanceChange_ != null)
            writeText("<p><font xsize=\"+1\">" + classDiff.inheritanceChange_ + "</font>");

        // Report changes in documentation
        if (reportDocChanges && classDiff.documentationChange_ != null) {
            String oldClassRef = null;
            if (oldDocPrefix != null) {
                oldClassRef = pkgName + "." + className;
                oldClassRef = oldClassRef.replace('.', '/');
                if (className.indexOf('.') != -1) {
                    oldClassRef = pkgName + ".";
                    oldClassRef = oldClassRef.replace('.', '/');
                    oldClassRef = oldDocPrefix + oldClassRef + className;
                } else {
                    oldClassRef = oldDocPrefix + oldClassRef;
                }
            }
            if (oldDocPrefix != null)
                classDiff.documentationChange_ += "<A HREF=\"" + oldClassRef +
                    ".html\" target=\"_self\"><code>old</code></A> to ";
            else
                classDiff.documentationChange_ += "<font size=\"+1\"><code>old</code></font> to ";
            classDiff.documentationChange_ += "<A HREF=\"" + classRef +
                ".html\" target=\"_self\"><code>new</code></A>. ";
            writeText(classDiff.documentationChange_);
        }

        if (classDiff.modifiersChange_ != null)
            writeText("<p>" + classDiff.modifiersChange_);

        reportAllCtors(pkgName, classDiff);
        reportAllMethods(pkgName, classDiff);
        reportAllFields(pkgName, classDiff);

        writeSectionFooter(pkgName, prevClassRef, nextClassRef, className, 2);
        writeHTMLFooter();
        reportFile.close();
        reportFile = oldReportFile;
    }

    /** 
     * Write out the details of constructors in a class. 
     */
    public void reportAllCtors(String pkgName, ClassDiff classDiff) {
        String className = classDiff.name_;
        writeText("<a NAME=\"constructors\"></a>"); // Named anchor
        // Report ctors which were removed in the new API
        if (classDiff.ctorsRemoved.size() != 0) {
            writeTableStart("Removed Constructors", 2);
            Iterator iter = classDiff.ctorsRemoved.iterator();
            while (iter.hasNext()) {
                ConstructorAPI ctorAPI = (ConstructorAPI)(iter.next());
                String ctorType = ctorAPI.getSignature();
                if (ctorType.compareTo("void") == 0)
                    ctorType = "";
                String id = className + "(" + ctorType + ")";
                if (trace) System.out.println("Constructor " + id + " was removed.");
                writeCtorTableEntry(pkgName, className, ctorType, 0, ctorAPI.doc_, false);
            }
            writeTableEnd();
        }

        // Report ctors which were added in the new API
        if (classDiff.ctorsAdded.size() != 0) {
            writeTableStart("Added Constructors", 2);
            Iterator iter = classDiff.ctorsAdded.iterator();
            while (iter.hasNext()) {
                ConstructorAPI ctorAPI = (ConstructorAPI)(iter.next());
                String ctorType = ctorAPI.getSignature();
                if (ctorType.compareTo("void") == 0)
                    ctorType = "";
                String id = className + "(" + ctorType + ")";
                if (trace) System.out.println("Constructor " + id + " was added.");
                writeCtorTableEntry(pkgName, className, ctorType, 1, ctorAPI.doc_, false);
            }
            writeTableEnd();
        }
        
        // Report ctors which were changed in the new API
        if (classDiff.ctorsChanged.size() != 0) {
            // Emit a table of changed classes, with links to the section
            // for each class.
            writeTableStart("Changed Constructors", 3);
            Iterator iter = classDiff.ctorsChanged.iterator();
            while (iter.hasNext()) {
                MemberDiff memberDiff = (MemberDiff)(iter.next());
                if (trace) System.out.println("Constructor for " + className +
                    " was changed from " + memberDiff.oldType_ + " to " + 
                    memberDiff.newType_);
                writeCtorChangedTableEntry(pkgName, className, memberDiff);
            }
            writeTableEnd();
        }
    }

    /** 
     * Write out the details of methods in a class. 
     */
    public void reportAllMethods(String pkgName, ClassDiff classDiff) {
        writeText("<a NAME=\"methods\"></a>"); // Named anchor
        String className = classDiff.name_;
        // Report methods which were removed in the new API
        if (classDiff.methodsRemoved.size() != 0) {
            writeTableStart("Removed Methods", 2);
            Iterator iter = classDiff.methodsRemoved.iterator();
            while (iter.hasNext()) {
                MethodAPI methodAPI = (MethodAPI)(iter.next());
                String methodName = methodAPI.name_ + "(" + methodAPI.getSignature() + ")";
                if (trace) System.out.println("Method " + methodName + " was removed.");
                writeMethodTableEntry(pkgName, className, methodAPI, 0, methodAPI.doc_, false);
            }
            writeTableEnd();
        }

        // Report methods which were added in the new API
        if (classDiff.methodsAdded.size() != 0) {
            writeTableStart("Added Methods", 2);
            Iterator iter = classDiff.methodsAdded.iterator();
            while (iter.hasNext()) {
                MethodAPI methodAPI = (MethodAPI)(iter.next());
                String methodName = methodAPI.name_ + "(" + methodAPI.getSignature() + ")";
                if (trace) System.out.println("Method " + methodName + " was added.");
                writeMethodTableEntry(pkgName, className, methodAPI, 1, methodAPI.doc_, false);
            }
            writeTableEnd();
        }
        
        // Report methods which were changed in the new API
        if (classDiff.methodsChanged.size() != 0) {
            // Emit a table of changed methods.
            writeTableStart("Changed Methods", 3);
            Iterator iter = classDiff.methodsChanged.iterator();
            while (iter.hasNext()) {
                MemberDiff memberDiff = (MemberDiff)(iter.next());
                if (trace) System.out.println("Method " + memberDiff.name_ + 
                      " was changed.");
                writeMethodChangedTableEntry(pkgName, className, memberDiff);
            }
            writeTableEnd();
        }
    }

    /** 
     * Write out the details of fields in a class. 
     */
    public void reportAllFields(String pkgName, ClassDiff classDiff) {
        writeText("<a NAME=\"fields\"></a>"); // Named anchor
        String className = classDiff.name_;
        // Report fields which were removed in the new API
        if (classDiff.fieldsRemoved.size() != 0) {
            writeTableStart("Removed Fields", 2);
            Iterator iter = classDiff.fieldsRemoved.iterator();
            while (iter.hasNext()) {
                FieldAPI fieldAPI = (FieldAPI)(iter.next());
                String fieldName = fieldAPI.name_;
                if (trace) System.out.println("Field " + fieldName + " was removed.");
                writeFieldTableEntry(pkgName, className, fieldAPI, 0, fieldAPI.doc_, false);
            }
            writeTableEnd();
        }
        
        // Report fields which were added in the new API
        if (classDiff.fieldsAdded.size() != 0) {
            writeTableStart("Added Fields", 2);
            Iterator iter = classDiff.fieldsAdded.iterator();
            while (iter.hasNext()) {
                FieldAPI fieldAPI = (FieldAPI)(iter.next());
                String fieldName = fieldAPI.name_;
                if (trace) System.out.println("Field " + fieldName + " was added.");
                writeFieldTableEntry(pkgName, className, fieldAPI, 1, fieldAPI.doc_, false);
            }
            writeTableEnd();
        }
        
        // Report fields which were changed in the new API
        if (classDiff.fieldsChanged.size() != 0) {
            // Emit a table of changed classes, with links to the section
            // for each class.
            writeTableStart("Changed Fields", 3);
            Iterator iter = classDiff.fieldsChanged.iterator();
            while (iter.hasNext()) {
                MemberDiff memberDiff = (MemberDiff)(iter.next());
                if (trace) System.out.println("Field " + pkgName + "." + className + "." + memberDiff.name_ + " was changed from " + memberDiff.oldType_ + " to " + memberDiff.newType_);
                writeFieldChangedTableEntry(pkgName, className, memberDiff);
            }
            writeTableEnd();
        }
        
    }

    /** 
     * Write the start of the HTML header, together with the current
     * date and time in an HTML comment. 
     */
    public void writeStartHTMLHeaderWithDate() {
        writeStartHTMLHeader(true);
    }

    /** Write the start of the HTML header. */
    public void writeStartHTMLHeader() {
        writeStartHTMLHeader(false);
    }

    /** Write the start of the HTML header. */
    public void writeStartHTMLHeader(boolean addDate) {
        writeText("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"https://www.w3.org/TR/html4/strict.dtd\">");
        writeText("<HTML style=\"overflow:auto;\">");
        writeText("<HEAD>");
        writeText("<meta name=\"generator\" content=\"JDiff v" + JDiff.version + "\">");
        writeText("<!-- Generated by the JDiff Javadoc doclet -->");
        writeText("<!-- (" + JDiff.jDiffLocation + ") -->");
        if (addDate)
            writeText("<!-- on " + new Date() + " -->");
        writeText("<meta name=\"description\" content=\"" + JDiff.jDiffDescription + "\">");
        writeText("<meta name=\"keywords\" content=\"" + JDiff.jDiffKeywords + "\">");
    }

    /** Write the HTML title */
    public void writeHTMLTitle(String title) {
        writeText("<TITLE>");
        writeText(title);
        writeText("</TITLE>");
    }

    /** 
     * Write the HTML style sheet reference for files in the subdirectory.
     */
    public void writeStyleSheetRef() {
        writeStyleSheetRef(false);
    }

    /**
     * Write the HTML style sheet reference. If inSameDir is set, don't add
     * "../" to the location.
     */

    public void writeStyleSheetRef(boolean inSameDir) {
        if (inSameDir) {
            writeText("<link href=\"../../../assets/android-developer-docs.css\" rel=\"stylesheet\" type=\"text/css\" />");
            writeText("<link href=\"../../assets/android-developer-docs.css\" rel=\"stylesheet\" type=\"text/css\" />");
            writeText("<link href=\"stylesheet-jdiff.css\" rel=\"stylesheet\" type=\"text/css\" />");
            writeText("<noscript>");
            writeText("<style type=\"text/css\">");
            writeText("body{overflow:auto;}");
            writeText("#body-content{position:relative; top:0;}");
            writeText("#doc-content{overflow:visible;border-left:3px solid #666;}");
            writeText("#side-nav{padding:0;}");
            writeText("#side-nav .toggle-list ul {display:block;}");
            writeText("#resize-packages-nav{border-bottom:3px solid #666;}");
            writeText("</style>");
            writeText("</noscript>");
            writeText("<style type=\"text/css\">");
            writeText("</style>");
	    } else {
            writeText("<link href=\"../../../../assets/android-developer-docs.css\" rel=\"stylesheet\" type=\"text/css\" />");
            writeText("<link href=\"../../../assets/android-developer-docs.css\" rel=\"stylesheet\" type=\"text/css\" />");
            writeText("<link href=\"../stylesheet-jdiff.css\" rel=\"stylesheet\" type=\"text/css\" />");
            writeText("<noscript>");
            writeText("<style type=\"text/css\">");
            writeText("body{overflow:auto;}");
            writeText("#body-content{position:relative; top:0;}");
            writeText("#doc-content{overflow:visible;border-left:3px solid #666;}");
            writeText("#side-nav{padding:0;}");
            writeText("#side-nav .toggle-list ul {display:block;}");
            writeText("#resize-packages-nav{border-bottom:3px solid #666;}");
            writeText("</style>");
            writeText("</noscript>");
            writeText("<style type=\"text/css\">");
            writeText("</style>");
	    }
    }

    /** Write the HTML footer. */
    public void writeHTMLFooter() {
        writeText("<script src=\"https://www.google-analytics.com/ga.js\" type=\"text/javascript\">");
        writeText("</script>");
        writeText("<script type=\"text/javascript\">");
        writeText("  try {");
        writeText("    var pageTracker = _gat._getTracker(\"UA-5831155-1\");");
        writeText("    pageTracker._setAllowAnchor(true);");
        writeText("    pageTracker._initData();");
        writeText("    pageTracker._trackPageview();");
        writeText("  } catch(e) {}");
        writeText("</script>");
        writeText("</BODY>");
        writeText("</HTML>");
    }

    /**
     * Write a section header, which includes a navigation bar.
     *
     * @param title Title of the header. Contains any links necessary.
     * @param packageName The name of the current package, with no slashes or
     *                    links in it. May be null
     * @param prevElemLink An HTML link to the previous element (a package or
     *                     class). May be null.
     * @param nextElemLink An HTML link to the next element (a package or
     *                     class). May be null.
     * @param className The name of the current class, with no slashes or
     *                  links in it. May be null.
     * @param level 0 = overview, 1 = package, 2 = class/interface
     */
    public void writeSectionHeader(String title, String packageName, 
                                   String prevElemLink, String nextElemLink,
                                   String className, int level, 
                                   boolean hasRemovals, 
                                   boolean hasAdditions, 
                                   boolean hasChanges) {
        writeNavigationBar(packageName, prevElemLink, nextElemLink,
                           className, level, true,
                           hasRemovals, hasAdditions, hasChanges);
        if (level != 0) {
            reportFile.println("<H2>");
            reportFile.println(title);
            reportFile.println("</H2>");
        }
    }
    
    /** 
     * Write a section footer, which includes a navigation bar. 
     * 
     * @param packageName The name of the current package, with no slashes or 
     *                    links in it. may be null
     * @param prevElemLink An HTML link to the previous element (a package or 
     *                     class). May be null.
     * @param nextElemLink An HTML link to the next element (a package or 
     *                     class). May be null.
     * @param className The name of the current class, with no slashes or 
     *                  links in it. May be null
     * @param level 0 = overview, 1 = package, 2 = class/interface
     */
    public void writeSectionFooter(String packageName,
                                   String prevElemLink, String nextElemLink,
                                   String className, int level) {
        writeText("      </div>	");
        writeText("      <div id=\"footer\">");
        writeText("        <div id=\"copyright\">");
        writeText("        Except as noted, this content is licensed under ");
        writeText("        <a href=\"https://creativecommons.org/licenses/by/2.5/\"> Creative Commons Attribution 2.5</a>.");
        writeText("        For details and restrictions, see the <a href=\"https://developer.android.com/license.html\">Content License</a>.");
        writeText("        </div>");
        writeText("      <div id=\"footerlinks\">");
        writeText("      <p>");
        writeText("        <a href=\"https://www.android.com/terms.html\">Site Terms of Service</a> -");
        writeText("        <a href=\"https://www.android.com/privacy.html\">Privacy Policy</a> -");
        writeText("        <a href=\"https://www.android.com/branding.html\">Brand Guidelines</a>");
        writeText("      </p>");
        writeText("    </div>");
        writeText("    </div> <!-- end footer -->");
        writeText("    </div><!-- end doc-content -->");
        writeText("    </div> <!-- end body-content --> ");

    }

    /**
     * Write a navigation bar section header.
     *
     * @param pkgName The name of the current package, with no slashes or
     *                links in it.
     * @param prevElemLink An HTML link to the previous element (a package or
     *                     class). May be null.
     * @param nextElemLink An HTML link to the next element (a package or
     *                     class). May be null.
     * @param className The name of the current class, with no slashes or
     *                links in it. May be null.
     * @param level 0 = overview, 1 = package, 2 = class/interface
     */

    public void writeNavigationBar(String pkgName,
                                   String prevElemLink, String nextElemLink,
                                   String className, int level,
                                   boolean upperNavigationBar,
                                   boolean hasRemovals, boolean hasAdditions,
                                   boolean hasChanges) {

        String oldAPIName = "Old API";
        if (apiDiff.oldAPIName_ != null)
            oldAPIName = apiDiff.oldAPIName_;
        String newAPIName = "New API";
        if (apiDiff.newAPIName_ != null)
            newAPIName = apiDiff.newAPIName_;

        SimpleDateFormat formatter
          = new SimpleDateFormat ("yyyy.MM.dd HH:mm");
        Date day = new Date();

	    reportFile.println("<!-- Start of nav bar -->");

	    reportFile.println("<a name=\"top\"></a>");
	    reportFile.println("<div id=\"header\" style=\"margin-bottom:0;padding-bottom:0;\">");
	    reportFile.println("<div id=\"headerLeft\">");
	    reportFile.println("<a href=\"../../../../index.html\" tabindex=\"-1\" target=\"_top\"><img src=\"../../../../assets/images/bg_logo.png\" alt=\"Android Developers\" /></a>");
	    reportFile.println("</div>");
	    reportFile.println("  <div id=\"headerRight\">");
	    reportFile.println("  <div id=\"headerLinks\">");
        reportFile.println("<!-- <img src=\"/assets/images/icon_world.jpg\" alt=\"\" /> -->");
	    reportFile.println("<span class=\"text\">");
	    reportFile.println("<!-- &nbsp;<a href=\"#\">English</a> | -->");
	    reportFile.println("<nobr><a href=\"https://developer.android.com\" target=\"_top\">Android Developers</a> | <a href=\"https://www.android.com\" target=\"_top\">Android.com</a></nobr>");
	    reportFile.println("</span>");
	    reportFile.println("</div>");
	    reportFile.println("  <div class=\"and-diff-id\" style=\"margin-top:6px;margin-right:8px;\">");
        reportFile.println("    <table class=\"diffspectable\">");
	    reportFile.println("      <tr>");
	    reportFile.println("        <td colspan=\"2\" class=\"diffspechead\">API Diff Specification</td>");
	    reportFile.println("      </tr>");
	    reportFile.println("      <tr>");
	    reportFile.println("        <td class=\"diffspec\" style=\"padding-top:.25em\">To Level:</td>");
	    reportFile.println("        <td class=\"diffvaluenew\" style=\"padding-top:.25em\">" + newAPIName + "</td>");
	    reportFile.println("      </tr>");
	    reportFile.println("      <tr>");
	    reportFile.println("        <td class=\"diffspec\">From Level:</td>");
	    reportFile.println("        <td class=\"diffvalueold\">" + oldAPIName + "</td>");
	    reportFile.println("      </tr>");
	    reportFile.println("      <tr>");
	    reportFile.println("        <td class=\"diffspec\">Generated</td>");
	    reportFile.println("        <td class=\"diffvalue\">" + formatter.format( day ) + "</td>");
	    reportFile.println("      </tr>");
 	    reportFile.println("    </table>");
 	    reportFile.println("    </div><!-- End and-diff-id -->");
        if (doStats) {
	    	reportFile.println("  <div class=\"and-diff-id\" style=\"margin-right:8px;\">");
	    	reportFile.println("    <table class=\"diffspectable\">");
	    	reportFile.println("      <tr>");
	    	reportFile.println("        <td class=\"diffspec\" colspan=\"2\"><a href=\"jdiff_statistics.html\">Statistics</a>");
	    	reportFile.println("      </tr>");
 	    	reportFile.println("    </table>");
	    	reportFile.println("  </div> <!-- End and-diff-id -->");
	    }
	    reportFile.println("  </div> <!-- End headerRight -->");
	    reportFile.println("  </div> <!-- End header -->");
	    reportFile.println("<div id=\"body-content\" xstyle=\"padding:12px;padding-right:18px;\">");
	    reportFile.println("<div id=\"doc-content\" style=\"position:relative;\">");
	    reportFile.println("<div id=\"mainBodyFluid\">");
    }

    /** Write the start of a table. */
    public void writeTableStart(String title, int colSpan) {
        reportFile.println("<p>");
        // Assumes that the first word of the title categorizes the table type
        // and that there is a space after the first word in the title
        int idx = title.indexOf(' ');
        String namedAnchor = title.substring(0, idx);
        reportFile.println("<a NAME=\"" + namedAnchor + "\"></a>"); // Named anchor
        reportFile.println("<TABLE summary=\"" + title+ "\" WIDTH=\"100%\">");
        reportFile.println("<TR>");
        reportFile.print("  <TH VALIGN=\"TOP\" COLSPAN=" + colSpan + ">");
        reportFile.println(title + "</FONT></TD>");
        reportFile.println("</TH>");
    }

    /** 
     * If a class or package name is considered to be too long for convenient
     * display, insert <br> in the middle of it at a period.
     */
    public String makeTwoRows(String name) {
        if (name.length() < 30)
            return name;
        int idx = name.indexOf(".", 20);
        if (idx == -1)
            return name;
        int len = name.length();
        String res = name.substring(0, idx+1) + "<br>" + name.substring(idx+1, len);
        return res;
    }

    /** 
     * Write a table entry for a package, with support for links to Javadoc 
     * for removed packages. 
     *
     * linkType: 0 - no link by default, 1 = link to Javadoc HTML file, 2 = link to JDiff file
     */
    public void writePackageTableEntry(String pkgName, int linkType, 
                                       String possibleComment, boolean useOld) {
        if (!useOld) {
            reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");
            reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
            reportFile.println("  <A NAME=\"" + pkgName + "\"></A>"); // Named anchor
        }
        //String shownPkgName = makeTwoRows(pkgName);
        if (linkType == 0) {
            if (oldDocPrefix == null) {
                // No link
                reportFile.print("  " + pkgName);
            } else {
                // Call this method again but this time to emit a link to the 
                // old program element.
                writePackageTableEntry(pkgName, 1, possibleComment, true);
            }
        } else if (linkType == 1) {
            // Link to HTML file for the package
            String pkgRef = pkgName;
            pkgRef = pkgRef.replace('.', '/');
            if (useOld)
                pkgRef = oldDocPrefix + pkgRef + "/package-summary";
            else
                pkgRef = newDocPrefix + pkgRef + "/package-summary";
            reportFile.println("  <nobr><A HREF=\"" + pkgRef + ".html\" target=\"_top\"><code>" + pkgName + "</code></A></nobr>");
        } else if (linkType == 2) {
            reportFile.println("  <nobr><A HREF=\"pkg_" + pkgName + reportFileExt + "\">" + pkgName + "</A></nobr>");
        }
        if (!useOld) {
            reportFile.println("  </TD>");
            emitComment(pkgName, possibleComment, linkType);
            reportFile.println("</TR>");
        }
    }

    /** 
     * Write a table entry for a class or interface. 
     *
     * linkType: 0 - no link by default, 1 = link to Javadoc HTML file, 2 = link to JDiff file
     */
    public void writeClassTableEntry(String pkgName, String className, 
                                     int linkType, boolean isInterface, 
                                     String possibleComment, boolean useOld) {
        if (!useOld) {
            reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");
            reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
            reportFile.println("  <A NAME=\"" + className + "\"></A>"); // Named anchor
        }
        String fqName = pkgName + "." + className;
        String shownClassName = makeTwoRows(className);
        if (linkType == 0) {
            if (oldDocPrefix == null) {
                // No link
                if (isInterface)
                    reportFile.println("  <I>" + shownClassName + "</I>");
                else
                    reportFile.println("  " + shownClassName);
            } else {
                writeClassTableEntry(pkgName, className, 
                                     1, isInterface, 
                                     possibleComment, true);
            }
        } else if (linkType == 1) {
            // Link to HTML file for the class
            String classRef = fqName;
            // Deal with inner classes
            if (className.indexOf('.') != -1) {
                classRef = pkgName + ".";
                classRef = classRef.replace('.', '/');
                if (useOld)
                    classRef = oldDocPrefix + classRef + className;
                else
                    classRef = newDocPrefix + classRef + className;
            } else {
                classRef = classRef.replace('.', '/');
                if (useOld)
                    classRef = oldDocPrefix + classRef;
                else
                    classRef = newDocPrefix + classRef;
            }
            reportFile.print("  <nobr><A HREF=\"" + classRef + ".html\" target=\"_top\"><code>");
            if (isInterface)
                reportFile.print("<I>" + shownClassName + "</I>");
            else
                reportFile.print(shownClassName);
            reportFile.println("</code></A></nobr>");
        } else if (linkType == 2) {
            reportFile.print("  <nobr><A HREF=\"" + fqName + reportFileExt + "\">");
            if (isInterface)
                reportFile.print("<I>" + shownClassName + "</I>");
            else
                reportFile.print(shownClassName);
            reportFile.println("</A></nobr>");
        }
        if (!useOld) {
            reportFile.println("  </TD>");
            emitComment(fqName, possibleComment, linkType);
            reportFile.println("</TR>");
        }
    }

    /** 
     * Write a table entry for a constructor. 
     *
     * linkType: 0 - no link by default, 1 = link to Javadoc HTML file
     */
    public void writeCtorTableEntry(String pkgName, String className, 
                                    String type, int linkType, 
                                    String possibleComment, boolean useOld) {
        String fqName = pkgName + "." + className;
        String shownClassName = makeTwoRows(className);
        String lt = "removed";
        if (linkType ==1)
            lt = "added";
        String commentID = fqName + ".ctor_" + lt + "(" + type + ")";
        if (!useOld) {
            reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");
            reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
            reportFile.println("  <A NAME=\"" + commentID + "\"></A>"); // Named anchor
        }
        String shortType = simpleName(type);
        if (linkType == 0) {
            if (oldDocPrefix == null) {
                // No link
                reportFile.print("  <nobr>" + className);
                emitTypeWithParens(shortType);
                reportFile.println("</nobr>");
            } else {
                writeCtorTableEntry(pkgName, className,
                                    type, 1,
                                    possibleComment, true);
            }
        } else if (linkType == 1) {
            // Link to HTML file for the package
            String memberRef = fqName.replace('.', '/');
            // Deal with inner classes
            if (className.indexOf('.') != -1) {
                memberRef = pkgName + ".";
                memberRef = memberRef.replace('.', '/');
                if (useOld) {
                    // oldDocPrefix is non-null at this point
                    memberRef = oldDocPrefix + memberRef + className;
                } else {
                    memberRef = newDocPrefix + memberRef + className;
                }
            } else {
                if (useOld) {
                    // oldDocPrefix is non-null at this point
                    memberRef = oldDocPrefix + memberRef;
                } else {
                    memberRef = newDocPrefix + memberRef;
                }
            }
            reportFile.print("  <nobr><A HREF=\"" + memberRef + ".html#" + className +
                             "(" + type + ")\" target=\"_top\"><code>" + shownClassName + "</code></A>");
            emitTypeWithParens(shortType);
            reportFile.println("</nobr>");
        }
        if (!useOld) {
            reportFile.println("  </TD>");
            emitComment(commentID, possibleComment, linkType);
            reportFile.println("</TR>");
        }
    }

    /** 
     * Write a table entry for a changed constructor.
     */
    public void writeCtorChangedTableEntry(String pkgName, String className, 
                                           MemberDiff memberDiff) {
        String fqName = pkgName + "." + className;
        String newSignature = memberDiff.newType_;
        if (newSignature.compareTo("void") == 0)
            newSignature = "";
        String commentID = fqName + ".ctor_changed(" + newSignature + ")";
        reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");
        reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
        reportFile.println("  <A NAME=\"" + commentID + "\"></A>"); // Named anchor
        String memberRef = fqName.replace('.', '/');            
        String shownClassName = makeTwoRows(className);
        // Deal with inner classes
        if (className.indexOf('.') != -1) {
            memberRef = pkgName + ".";
            memberRef = memberRef.replace('.', '/');
            memberRef = newDocPrefix + memberRef + className;
        } else {
            memberRef = newDocPrefix + memberRef;
        }
        String newType = memberDiff.newType_;
        if (newType.compareTo("void") == 0)
            newType = "";
        String shortNewType = simpleName(memberDiff.newType_);
        // Constructors have the linked name, then the type in parentheses.
        reportFile.print("  <nobr><A HREF=\"" + memberRef + ".html#" + className + "(" + newType + ")\" target=\"_top\"><code>");
        reportFile.print(shownClassName);
        reportFile.print("</code></A>");
        emitTypeWithParens(shortNewType);
        reportFile.println("  </nobr>");
        reportFile.println("  </TD>");

        // Report changes in documentation
        if (reportDocChanges && memberDiff.documentationChange_ != null) {
            String oldMemberRef = null;
            String oldType = null;
            if (oldDocPrefix != null) {
                oldMemberRef = pkgName + "." + className;
                oldMemberRef = oldMemberRef.replace('.', '/');
                if (className.indexOf('.') != -1) {
                    oldMemberRef = pkgName + ".";
                    oldMemberRef = oldMemberRef.replace('.', '/');
                    oldMemberRef = oldDocPrefix + oldMemberRef + className;
                } else {
                    oldMemberRef = oldDocPrefix + oldMemberRef;
                }
                oldType = memberDiff.oldType_;
                if (oldType.compareTo("void") == 0)
                    oldType = "";
            }
            if (oldDocPrefix != null)
                memberDiff.documentationChange_ += "<A HREF=\"" +
                    oldMemberRef + ".html#" + className + "(" + oldType +
                    ")\" target=\"_self\"><code>old</code></A> to ";
            else 
                memberDiff.documentationChange_ += "<font size=\"+1\"><code>old</code></font> to ";
            memberDiff.documentationChange_ += "<A HREF=\"" + memberRef +
                ".html#" + className + "(" + newType +
                ")\" target=\"_self\"><code>new</code></A>.<br>";
        }

        emitChanges(memberDiff, 0);
        emitComment(commentID, null, 2);

        reportFile.println("</TR>");
    }

    /**
     * Write a table entry for a method.
     *
     * linkType: 0 - no link by default, 1 = link to Javadoc HTML file
     */
    public void writeMethodTableEntry(String pkgName, String className, 
                                      MethodAPI methodAPI, int linkType, 
                                      String possibleComment, boolean useOld) {
        String fqName = pkgName + "." + className;
        String signature = methodAPI.getSignature(); 
        String methodName = methodAPI.name_;
        String lt = "removed";
        if (linkType ==1)
            lt = "added";
        String commentID = fqName + "." + methodName + "_" + lt + "(" + signature + ")";
        if (!useOld) {
            reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");
            reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
            reportFile.println("  <A NAME=\"" + commentID + "\"></A>"); // Named anchor
        }
        if (signature.compareTo("void") == 0)
            signature = "";
        String shortSignature = simpleName(signature);
        String returnType = methodAPI.returnType_;
        String shortReturnType = simpleName(returnType);
        if (linkType == 0) {
            if (oldDocPrefix == null) {
                // No link
                reportFile.print("  <nobr>");
                emitType(shortReturnType);
                reportFile.print("&nbsp;" + methodName);
                emitTypeWithParens(shortSignature);
                reportFile.println("</nobr>");
            } else {
                writeMethodTableEntry(pkgName, className, 
                                      methodAPI, 1, 
                                      possibleComment, true);
            }
        } else if (linkType == 1) {
            // Link to HTML file for the package
            String memberRef = fqName.replace('.', '/');
            // Deal with inner classes
            if (className.indexOf('.') != -1) {
                memberRef = pkgName + ".";
                memberRef = memberRef.replace('.', '/');
                if (useOld) {
                    // oldDocPrefix is non-null at this point
                    memberRef = oldDocPrefix + memberRef + className;
                } else {
                    memberRef = newDocPrefix + memberRef + className;
                }
            } else {
                if (useOld) {
                    // oldDocPrefix is non-null at this point
                    memberRef = oldDocPrefix + memberRef;
                } else {
                    memberRef = newDocPrefix + memberRef;
                }
            }
            reportFile.print("  <nobr>");
            emitType(shortReturnType);
            reportFile.print("&nbsp;<A HREF=\"" + memberRef + ".html#" + methodName +
               "(" + signature + ")\" target=\"_top\"><code>" + methodName + "</code></A>");
            emitTypeWithParens(shortSignature);
            reportFile.println("</nobr>");
        }
        if (!useOld) {
            reportFile.println("  </TD>");
            emitComment(commentID, possibleComment, linkType);
            reportFile.println("</TR>");
        }
    }

    /**
     * Write a table entry for a changed method.
     */
    public void writeMethodChangedTableEntry(String pkgName, String className, 
                                      MemberDiff memberDiff) {
        String memberName = memberDiff.name_;
        // Generally nowhere to break a member name anyway
        // String shownMemberName = makeTwoRows(memberName);
        String fqName = pkgName + "." + className;
        String newSignature = memberDiff.newSignature_;
        String commentID = fqName + "." + memberName + "_changed(" + newSignature + ")";
        reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");

        reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
        reportFile.println("  <A NAME=\"" + commentID + "\"></A>"); // Named anchor
        String memberRef = fqName.replace('.', '/');            
        // Deal with inner classes
        if (className.indexOf('.') != -1) {
            memberRef = pkgName + ".";
            memberRef = memberRef.replace('.', '/');
            memberRef = newDocPrefix + memberRef + className;
        } else {
            memberRef = newDocPrefix + memberRef;
        }
        // Javadoc generated HTML has no named anchors for methods
        // inherited from other classes, so link to the defining class' method.
        // Only copes with non-inner classes.
        if (className.indexOf('.') == -1 &&
            memberDiff.modifiersChange_ != null &&
            memberDiff.modifiersChange_.indexOf("but is now inherited from") != -1) {
            memberRef = memberDiff.inheritedFrom_;
            memberRef = memberRef.replace('.', '/');
            memberRef = newDocPrefix + memberRef;
        }

        String newReturnType = memberDiff.newType_;
        String shortReturnType = simpleName(newReturnType);
        String shortSignature = simpleName(newSignature);
        reportFile.print("  <nobr>");
        emitTypeWithNoParens(shortReturnType);
        reportFile.print("&nbsp;<A HREF=\"" + memberRef + ".html#" +
                         memberName + "(" + newSignature + ")\" target=\"_top\"><code>");
        reportFile.print(memberName);
        reportFile.print("</code></A>");
        emitTypeWithParens(shortSignature);
        reportFile.println("  </nobr>");
        reportFile.println("  </TD>");

        // Report changes in documentation
        if (reportDocChanges && memberDiff.documentationChange_ != null) {
            String oldMemberRef = null;
            String oldSignature = null;
            if (oldDocPrefix != null) {
                oldMemberRef = pkgName + "." + className;
                oldMemberRef = oldMemberRef.replace('.', '/');
                if (className.indexOf('.') != -1) {
                    oldMemberRef = pkgName + ".";
                    oldMemberRef = oldMemberRef.replace('.', '/');
                    oldMemberRef = oldDocPrefix + oldMemberRef + className;
                } else {
                    oldMemberRef = oldDocPrefix + oldMemberRef;
                }
                oldSignature = memberDiff.oldSignature_;
            }
            if (oldDocPrefix != null)
                memberDiff.documentationChange_ += "<A HREF=\"" +
                    oldMemberRef + ".html#" + memberName + "(" +
                    oldSignature + ")\" target=\"_self\"><code>old</code></A> to ";
            else
                memberDiff.documentationChange_ += "<code>old</code> to ";
            memberDiff.documentationChange_ += "<A HREF=\"" + memberRef +
                ".html#" + memberName + "(" + newSignature +
                ")\" target=\"_self\"><code>new</code></A>.<br>";
        }

        emitChanges(memberDiff, 1);
        // Get the comment from the parent class if more appropriate
        if (memberDiff.modifiersChange_ != null) {
            int parentIdx = memberDiff.modifiersChange_.indexOf("now inherited from");
            if (parentIdx != -1) {
                // Change the commentID to pick up the appropriate method
                commentID = memberDiff.inheritedFrom_ + "." + memberName +
                    "_changed(" + newSignature + ")";
            }
        }
        emitComment(commentID, null, 2);
        
        reportFile.println("</TR>");
    }

    /** 
     * Write a table entry for a field. 
     *
     * linkType: 0 - no link by default, 1 = link to Javadoc HTML file
     */
    public void writeFieldTableEntry(String pkgName, String className, 
                                     FieldAPI fieldAPI, int linkType, 
                                     String possibleComment, boolean useOld) {
        String fqName = pkgName + "." + className;
        // Fields can only appear in one table, so no need to specify _added etc
        String fieldName = fieldAPI.name_;
        // Generally nowhere to break a member name anyway
        // String shownFieldName = makeTwoRows(fieldName);
        String commentID = fqName + "." + fieldName;
        if (!useOld) {
            reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");
            reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
            reportFile.println("  <A NAME=\"" + commentID + "\"></A>"); // Named anchor
        }
        String fieldType = fieldAPI.type_;
        if (fieldType.compareTo("void") == 0)
            fieldType = "";
        String shortFieldType = simpleName(fieldType);
        if (linkType == 0) {
            if (oldDocPrefix == null) {
                // No link.
                reportFile.print("  ");
                emitType(shortFieldType);
                reportFile.println("&nbsp;" + fieldName);
            } else {
                writeFieldTableEntry(pkgName, className, 
                                     fieldAPI, 1, 
                                     possibleComment, true);
            }
        } else if (linkType == 1) {
            // Link to HTML file for the package.
            String memberRef = fqName.replace('.', '/');
            // Deal with inner classes
            if (className.indexOf('.') != -1) {
                memberRef = pkgName + ".";
                memberRef = memberRef.replace('.', '/');
                if (useOld)
                    memberRef = oldDocPrefix + memberRef + className;
                else
                    memberRef = newDocPrefix + memberRef + className;
            } else {
                if (useOld)
                    memberRef = oldDocPrefix + memberRef;
                else
                    memberRef = newDocPrefix + memberRef;
            }
            reportFile.print("  <nobr>");
            emitType(shortFieldType);
            reportFile.println("&nbsp;<A HREF=\"" + memberRef + ".html#" + fieldName +
               "\" target=\"_top\"><code>" + fieldName + "</code></A></nobr>");
        }
        if (!useOld) {
            reportFile.println("  </TD>");
            emitComment(commentID, possibleComment, linkType);
            reportFile.println("</TR>");
        }
    }

    /**
     * Write a table entry for a changed field.
     */
    public void writeFieldChangedTableEntry(String pkgName, String className,
                                            MemberDiff memberDiff) {
        String memberName = memberDiff.name_;
        // Generally nowhere to break a member name anyway
        // String shownMemberName = makeTwoRows(memberName);
        String fqName = pkgName + "." + className;
        // Fields have unique names in a class
        String commentID = fqName + "." + memberName;
        reportFile.println("<TR BGCOLOR=\"" + bgcolor + "\" CLASS=\"TableRowColor\">");

        reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"25%\">");
        reportFile.println("  <A NAME=\"" + commentID + "\"></A>"); // Named anchor
        String memberRef = fqName.replace('.', '/');            
        // Deal with inner classes
        if (className.indexOf('.') != -1) {
            memberRef = pkgName + ".";
            memberRef = memberRef.replace('.', '/');
            memberRef = newDocPrefix + memberRef + className;
        } else {
            memberRef = newDocPrefix + memberRef;
        }
        // Javadoc generated HTML has no named anchors for fields
        // inherited from other classes, so link to the defining class' field.
        // Only copes with non-inner classes.
        if (className.indexOf('.') == -1 &&
            memberDiff.modifiersChange_ != null &&
            memberDiff.modifiersChange_.indexOf("but is now inherited from") != -1) {
            memberRef = memberDiff.inheritedFrom_;
            memberRef = memberRef.replace('.', '/');
            memberRef = newDocPrefix + memberRef;
        }

        String newType = memberDiff.newType_;
        String shortNewType = simpleName(newType);
        reportFile.print("  <nobr>");
        emitTypeWithNoParens(shortNewType);
        reportFile.print("&nbsp;<A HREF=\"" + memberRef + ".html#" +
                         memberName + "\" target=\"_top\"><code>");
        reportFile.print(memberName);
        reportFile.print("</code></font></A></nobr>");
        reportFile.println("  </TD>");

        // Report changes in documentation
        if (reportDocChanges && memberDiff.documentationChange_ != null) {
            String oldMemberRef = null;
            if (oldDocPrefix != null) {
                oldMemberRef = pkgName + "." + className;
                oldMemberRef = oldMemberRef.replace('.', '/');
                if (className.indexOf('.') != -1) {
                    oldMemberRef = pkgName + ".";
                    oldMemberRef = oldMemberRef.replace('.', '/');
                    oldMemberRef = oldDocPrefix + oldMemberRef + className;
                } else {
                    oldMemberRef = oldDocPrefix + oldMemberRef;
                }
            }
            if (oldDocPrefix != null)
                memberDiff.documentationChange_ += "<A HREF=\"" +
                    oldMemberRef + ".html#" + memberName + "\" target=\"_self\"><code>old</code></A> to ";
            else
                memberDiff.documentationChange_ += "<code>old</code> to ";
            memberDiff.documentationChange_ += "<A HREF=\"" + memberRef +
                ".html#" + memberName + "\" target=\"_self\"><code>new</code></A>.<br>";
        }

        emitChanges(memberDiff, 2);
        // Get the comment from the parent class if more appropriate
        if (memberDiff.modifiersChange_ != null) {
            int parentIdx = memberDiff.modifiersChange_.indexOf("now inherited from");
            if (parentIdx != -1) {
                // Change the commentID to pick up the appropriate method
                commentID = memberDiff.inheritedFrom_ + "." + memberName;
            }
        }
        emitComment(commentID, null, 2);
        
        reportFile.println("</TR>");
    }

    /**
     * Emit all changes associated with a MemberDiff as an entry in a table.
     *
     * @param memberType 0 = ctor, 1 = method, 2 = field
     */
    public void emitChanges(MemberDiff memberDiff, int memberType){
        reportFile.println("  <TD VALIGN=\"TOP\" WIDTH=\"30%\">");
        boolean hasContent = false;
        // The type or return type changed
        if (memberDiff.oldType_.compareTo(memberDiff.newType_) != 0) {
            String shortOldType = simpleName(memberDiff.oldType_);
            String shortNewType = simpleName(memberDiff.newType_);
            if (memberType == 1) {
                reportFile.print("Change in return type from ");
            } else {
                reportFile.print("Change in type from ");
            }
            if (shortOldType.compareTo(shortNewType) == 0) {
                // The types differ in package name, so use the full name
                shortOldType = memberDiff.oldType_;
                shortNewType = memberDiff.newType_;
            }
            emitType(shortOldType);
            reportFile.print(" to ");
            emitType(shortNewType);
            reportFile.println(".<br>");
            hasContent = true;
        }
        // The signatures changed - only used by methods
        if (memberType == 1 &&
            memberDiff.oldSignature_ != null && 
            memberDiff.newSignature_ != null && 
            memberDiff.oldSignature_.compareTo(memberDiff.newSignature_) != 0) {
            String shortOldSignature = simpleName(memberDiff.oldSignature_);
            String shortNewSignature = simpleName(memberDiff.newSignature_);
            if (shortOldSignature.compareTo(shortNewSignature) == 0) {
                // The signatures differ in package names, so use the full form
                shortOldSignature = memberDiff.oldSignature_;
                shortNewSignature = memberDiff.newSignature_;
            }
            if (hasContent)
                reportFile.print(" "); 
            reportFile.print("Change in signature from ");
            if (shortOldSignature.compareTo("") == 0)
                shortOldSignature = "void";
            emitType(shortOldSignature);
            reportFile.print(" to ");
            if (shortNewSignature.compareTo("") == 0)
                shortNewSignature = "void";
            emitType(shortNewSignature);
            reportFile.println(".<br>");
            hasContent = true;
        }
        // The exceptions are only non-null in methods and constructors
        if (memberType != 2 &&
            memberDiff.oldExceptions_ != null && 
            memberDiff.newExceptions_ != null && 
            memberDiff.oldExceptions_.compareTo(memberDiff.newExceptions_) != 0) {
            if (hasContent)
                reportFile.print(" "); 
            // If either one of the exceptions has no spaces in it, or is 
            // equal to "no exceptions", then just display the whole 
            // exceptions texts.
            int spaceInOld = memberDiff.oldExceptions_.indexOf(" ");
            if (memberDiff.oldExceptions_.compareTo("no exceptions") == 0)
                spaceInOld = -1;
            int spaceInNew = memberDiff.newExceptions_.indexOf(" ");
            if (memberDiff.newExceptions_.compareTo("no exceptions") == 0)
                spaceInNew = -1;
            if (spaceInOld == -1 || spaceInNew == -1) {
                reportFile.print("Change in exceptions thrown from ");
                emitException(memberDiff.oldExceptions_); 
                reportFile.print(" to " );
                emitException(memberDiff.newExceptions_); 
                reportFile.println(".<br>");
            } else {
                // Too many exceptions become unreadable, so just show the 
                // individual changes. Catch the case where exceptions are
                // just reordered.
                boolean firstChange = true;
                int numRemoved = 0;
                StringTokenizer stOld = new StringTokenizer(memberDiff.oldExceptions_, ", ");
                while (stOld.hasMoreTokens()) {
                    String oldException = stOld.nextToken();
                    if (!memberDiff.newExceptions_.startsWith(oldException) &&
                        !(memberDiff.newExceptions_.indexOf(", " + oldException) != -1)) {
                        if (firstChange) {
                            reportFile.print("Change in exceptions: ");
                            firstChange = false;
                        }
                        if (numRemoved != 0)
                            reportFile.print(", ");
                        emitException(oldException);
                        numRemoved++;
                    }
                }
                if (numRemoved == 1)
                    reportFile.print(" was removed.");
                else if (numRemoved > 1)
                    reportFile.print(" were removed.");
                
                int numAdded = 0;
                StringTokenizer stNew = new StringTokenizer(memberDiff.newExceptions_, ", ");
                while (stNew.hasMoreTokens()) {
                    String newException = stNew.nextToken();
                    if (!memberDiff.oldExceptions_.startsWith(newException) &&
                        !(memberDiff.oldExceptions_.indexOf(", " + newException) != -1)) {
                        if (firstChange) {
                            reportFile.print("Change in exceptions: ");
                            firstChange = false;
                        }
                        if (numAdded != 0)
                            reportFile.println(", ");    
                        else
                            reportFile.println(" ");
                        emitException(newException);
                        numAdded++;
                    }
                }
                if (numAdded == 1)
                    reportFile.print(" was added");
                else if (numAdded > 1)
                    reportFile.print(" were added");
                else if (numAdded == 0 && numRemoved == 0 && firstChange)
                    reportFile.print("Exceptions were reordered");
                reportFile.println(".<br>");
            }
            // Note the changes between a comma-separated list of Strings
            hasContent = true;
        }

        if (memberDiff.documentationChange_ != null) {
            if (hasContent)
                reportFile.print(" "); 
            reportFile.print(memberDiff.documentationChange_);
            hasContent = true;
        }
            
        // Last, so no need for a <br>
        if (memberDiff.modifiersChange_ != null) {
            if (hasContent)
                reportFile.print(" "); 
            reportFile.println(memberDiff.modifiersChange_);
            hasContent = true;
        }
        reportFile.println("  </TD>");
    }

    /** 
     * Emit a string which is an exception by surrounding it with 
     * &lt;code&gt; tags.
     * If there is a space in the type, e.g. &quot;String, File&quot;, then
     * surround it with parentheses too. Do not add &lt;code&gt; tags or 
     * parentheses if the String is "no exceptions".
     */
    public void emitException(String ex) {
        if (ex.compareTo("no exceptions") == 0) {
            reportFile.print(ex);
        } else {
            if (ex.indexOf(' ') != -1) {
                reportFile.print("(<code>" + ex + "</code>)");
            } else {
                reportFile.print("<code>" + ex + "</code>");
            }
        }
    }

    /** 
     * Emit a string which is a type by surrounding it with &lt;code&gt; tags.
     * If there is a space in the type, e.g. &quot;String, File&quot;, then
     * surround it with parentheses too.
     */
    public void emitType(String type) {
        if (type.compareTo("") == 0)
            return;
        if (type.indexOf(' ') != -1) {
            reportFile.print("(<code>" + type + "</code>)");
        } else {
            reportFile.print("<code>" + type + "</code>");
        }
    }

    /** 
     * Emit a string which is a type by surrounding it with &lt;code&gt; tags.
     * Also surround it with parentheses too. Used to display methods' 
     * parameters.
     * Suggestions for where a browser should break the 
     * text are provided with &lt;br> and &ltnobr> tags.
     */
    public static void emitTypeWithParens(String type) {
        emitTypeWithParens(type, true);
    }

    /** 
     * Emit a string which is a type by surrounding it with &lt;code&gt; tags.
     * Also surround it with parentheses too. Used to display methods' 
     * parameters.
     */
    public static void emitTypeWithParens(String type, boolean addBreaks) {
        if (type.compareTo("") == 0)
            reportFile.print("()");
        else {
            int idx = type.indexOf(", ");
            if (!addBreaks || idx == -1) {
                reportFile.print("(<code>" + type + "</code>)");
            } else {
                // Make the browser break text at reasonable places
                String sepType = null;
                StringTokenizer st = new StringTokenizer(type, ", ");
                while (st.hasMoreTokens()) {
                    String p = st.nextToken();
                    if (sepType == null)
                        sepType = p;
                    else
                        sepType += ",</nobr> " + p + "<nobr>";
                }
                reportFile.print("(<code>" + sepType + "<nobr></code>)");
            }
        }
    }

    /**
     * Emit a string which is a type by surrounding it with &lt;code&gt; tags.
     * Do not surround it with parentheses. Used to display methods' return
     * types and field types.
     */
    public static void emitTypeWithNoParens(String type) {
        if (type.compareTo("") != 0)
            reportFile.print("<code>" + type + "</code>");
    }

    /** 
     * Return a String with the simple names of the classes in fqName.
     * &quot;java.lang.String&quot; becomes &quot;String&quot;, 
     * &quotjava.lang.String, java.io.File&quot becomes &quotString, File&quot;
     * and so on. If fqName is null, return null. If fqName is &quot;&quot;, 
     * return &quot;&quot;. 
     */
    public static String simpleName(String fqNames) {
        if (fqNames == null)
            return null;
        String res = "";
        boolean hasContent = false;
        // We parse the string step by step to ensure we take
        // fqNames that contains generics parameter in a whole.
        ArrayList<String> fqNamesList = new ArrayList<String>();
        int genericParametersDepth = 0;
        StringBuffer buffer = new StringBuffer();
        for (int i=0; i<fqNames.length(); i++) {
          char c = fqNames.charAt(i);
          if ('<' == c) {
            genericParametersDepth++;
          }
          if ('>' == c) {
            genericParametersDepth--;
          }
          if (',' != c || genericParametersDepth > 0) {
            buffer.append(c);
          } else if (',' == c) {
            fqNamesList.add(buffer.toString().trim());
            buffer = new StringBuffer(buffer.length());
          }
        }
        fqNamesList.add(buffer.toString().trim());
        for (String fqName : fqNamesList) {
            // Assume this will be used inside a <nobr> </nobr> set of tags.
            if (hasContent)
                res += ", "; 
            hasContent = true;
            // Look for text within '<' and '>' in case this is a invocation of a generic
            
            int firstBracket = fqName.indexOf('<');
            int lastBracket = fqName.lastIndexOf('>');
            String genericParameter = null;
            if (firstBracket != -1 && lastBracket != -1) {
              genericParameter = simpleName(fqName.substring(firstBracket + 1, lastBracket));
              fqName = fqName.substring(0, firstBracket);              
            }
            
            int lastDot = fqName.lastIndexOf('.');
            if (lastDot < 0) {
                res += fqName; // Already as simple as possible
            } else {
                res += fqName.substring(lastDot+1);
            }
            if (genericParameter != null)
              res += "&lt;" + genericParameter + "&gt;";            
        }
        return res;
    }

    /** 
     * Find any existing comment and emit it. Add the new comment to the
     * list of new comments. The first instance of the string "@first" in 
     * a hand-written comment will be replaced by the first sentence from 
     * the associated doc block, if such exists. Also replace @link by
     * an HTML link.
     *
     * @param commentID The identifier for this comment.
     * @param possibleComment A possible comment from another source.
     * @param linkType 0 = remove, 1 = add, 2 = change
     */
    public void emitComment(String commentID, String possibleComment, 
                            int linkType) {
        if (noCommentsOnRemovals && linkType == 0) {
            reportFile.println("  <TD>&nbsp;</TD>");
            return;
        }
        if (noCommentsOnAdditions && linkType == 1) {
            reportFile.println("  <TD>&nbsp;</TD>");
            return;
        }
        if (noCommentsOnChanges && linkType == 2) {
            reportFile.println("  <TD>&nbsp;</TD>");
            return;
        }

        // We have to use this global hash table because the *Diff classes
        // do not store the possible comment from the new *API object.
        if (!noCommentsOnChanges && possibleComment == null) {
            possibleComment = (String)Comments.allPossibleComments.get(commentID);
        }
        // Just use the first sentence of the possible comment.
        if (possibleComment != null) {
            int fsidx = RootDocToXML.endOfFirstSentence(possibleComment, false);
            if (fsidx != -1 && fsidx != 0)
                possibleComment = possibleComment.substring(0, fsidx+1);
        }

        String comment = Comments.getComment(existingComments_, commentID);
        if (comment.compareTo(Comments.placeHolderText) == 0) {
            if (possibleComment != null && 
                possibleComment.indexOf("InsertOtherCommentsHere") == -1)
                reportFile.println("  <TD VALIGN=\"TOP\">" + possibleComment + "</TD>");
            else
                reportFile.println("  <TD>&nbsp;</TD>");
        } else {
            int idx = comment.indexOf("@first");
            if (idx == -1) {
                reportFile.println("  <TD VALIGN=\"TOP\">" + Comments.convertAtLinks(comment, "", null, null) + "</TD>");
            } else {
                reportFile.print("  <TD VALIGN=\"TOP\">" + comment.substring(0, idx));
                if (possibleComment != null && 
                    possibleComment.indexOf("InsertOtherCommentsHere") == -1)
                    reportFile.print(possibleComment);
                reportFile.println(comment.substring(idx + 6) + "</TD>");
            }
        }
        SingleComment newComment = new SingleComment(commentID, comment);
        newComments_.addComment(newComment);
    }
    
    /** Write the end of a table. */
    public void writeTableEnd() {
        reportFile.println("</TABLE>");
        reportFile.println("&nbsp;");
    } 

    /** Write a newline out. */
    public void writeText() {
        reportFile.println();
    } 

    /** Write some text out. */
    public void writeText(String text) {
        reportFile.println(text);
    } 

    /** Emit some non-breaking space for indentation. */
    public void indent(int indent) {
        for (int i = 0; i < indent; i++)
            reportFile.print("&nbsp;");
    } 

    /** 
     * The name of the file to which the top-level HTML file is written, 
     * and also the name of the subdirectory where most of the HTML appears,
     * and also a prefix for the names of some of the files in that 
     * subdirectory.
     */
    static String reportFileName = "changes";
    
    /** 
     * The suffix of the file to which the HTML output is currently being 
     * written. 
     */
    static String reportFileExt = ".html";
    
    /** 
     * The file to which the HTML output is currently being written. 
     */
    static PrintWriter reportFile = null;

    /** 
     * The object which represents the top of the tree of differences
     * between two APIs. It is only used indirectly when emitting a
     * navigation bar.
     */
    static APIDiff apiDiff = null;

    /** 
     * If set, then do not suggest comments for removals from the first 
     * sentence of the doc block of the old API. 
     */
    public static boolean noCommentsOnRemovals = false;

    /** 
     * If set, then do not suggest comments for additions from the first 
     * sentence of the doc block of the new API. 
     */
    public static boolean noCommentsOnAdditions = false;

    /** 
     * If set, then do not suggest comments for changes from the first 
     * sentence of the doc block of the new API. 
     */
    public static boolean noCommentsOnChanges = false;

    /**
     * If set, then report changes in documentation (Javadoc comments)
     * between the old and the new API. The default is that this is not set.
     */
    public static boolean reportDocChanges = false;

    /**
     * Define the prefix for HTML links to the existing set of Javadoc-
     * generated documentation for the new API. E.g. For J2SE1.3.x, use
     * "https://java.sun.com/j2se/1.3/docs/api/"
     */
    public static String newDocPrefix = "../";

    /**
     * Define the prefix for HTML links to the existing set of Javadoc-
     * generated documentation for the old API.
     */
    public static String oldDocPrefix = null;

    /** To generate statistical output, set this to true. */
    public static boolean doStats = false;

    /** 
     * The destination directory for output files.
     */
    public static String outputDir = null;
    
    /** 
     * The destination directory for comments files (if not specified, uses outputDir)
     */
    public static String commentsDir = null;

    /** 
     * The title used on the first page of the report. By default, this is 
     * &quot;API Differences Between &lt;name of old API&gt; and 
     * &lt;name of new API&gt;&quot;. It can be
     * set by using the -doctitle option.
     */
    public static String docTitle = null;

    /** 
     * The browser window title for the report. By default, this is 
     * &quot;API Differences Between &lt;name of old API&gt; and 
     * &lt;name of new API&gt;&quot;. It can be
     * set by using the -windowtitle option.
     */
    public static String windowTitle = null;

    /** The desired background color for JDiff tables. */
    static final String bgcolor = "#FFFFFF";

    /** Set to enable debugging output. */
    private static final boolean trace = false;

}
