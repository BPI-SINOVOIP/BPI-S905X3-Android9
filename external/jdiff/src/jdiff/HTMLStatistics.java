package jdiff;

import java.util.*;
import java.io.*;
import java.text.*;

/**
 * Emit an HTML file containing statistics about the differences.
 * Statistical information only appears if the -stats argument is used.
 *
 * See the file LICENSE.txt for copyright details.
 * @author Matthew Doar, mdoar@pobox.com
 */

public class HTMLStatistics {

    /** Constructor. */
    public HTMLStatistics(HTMLReportGenerator h) {
        h_ = h;
    }   

    /** The HTMLReportGenerator instance used to write HTML. */
    private HTMLReportGenerator h_ = null;

    /** 
     * Emit the statistics HTML file.
     */

    public void emitStatistics(String filename, APIDiff apiDiff) {
        try {
            FileOutputStream fos = new FileOutputStream(filename);
            h_.reportFile = new PrintWriter(fos);
            // Write out the HTML header
            h_.writeStartHTMLHeader();
            String oldAPIName = "Old API";
            if (apiDiff.oldAPIName_ != null)
                oldAPIName = apiDiff.oldAPIName_;
            String newAPIName = "New API";
            if (apiDiff.newAPIName_ != null)
                newAPIName = apiDiff.newAPIName_;
            // Write out the title
            h_.writeHTMLTitle("API Change Statistics");
            h_.writeStyleSheetRef();
            h_.writeText("</HEAD>");
            h_.writeText("<body class=\"gc-documentation\">");

           // writeText("<div class=\"g-section g-tpl-180\">");
           // Add the nav bar for the summary page


            // Write a customized navigation bar for the statistics page
            h_.writeText("<!-- Start of nav bar -->");

            SimpleDateFormat formatter
              = new SimpleDateFormat ("yyyy.MM.dd HH:mm");
            Date day = new Date();

	    h_.writeText("<a name=\"top\"></a>");
	    h_.writeText("<div id=\"header\" style=\"margin-bottom:0;xborder-bottom:none;\">");
	    h_.writeText("<div id=\"headerLeft\">");
	    h_.writeText("<a href=\"../../../../index.html\" tabindex=\"-1\" target=\"_top\"><img src=\"../../../../assets/images/bg_logo.png\" alt=\"Android Developers\" /></a>");
	    h_.writeText("</div>");
	    h_.writeText("  <div id=\"headerRight\">");
	    h_.writeText("  <div id=\"headerLinks\">");
            h_.writeText("<!-- <img src=\"/assets/images/icon_world.jpg\" alt=\"\" /> -->");
	    h_.writeText("<span class=\"text\">");
	    h_.writeText("<!-- &nbsp;<a href=\"#\">English</a> | -->");
	    h_.writeText("<nobr><a href=\"https://developer.android.com\" target=\"_top\">Android Developers</a> | <a href=\"https://www.android.com\" target=\"_top\">Android.com</a></nobr>");
	    h_.writeText("</span>");
	    h_.writeText("</div>");
	    h_.writeText("  <div class=\"and-diff-id\" style=\"margin-top:6px;margin-right:8px;\">");
            h_.writeText("    <table class=\"diffspectable\">");
	    h_.writeText("      <tr>");
	    h_.writeText("        <td colspan=\"2\" class=\"diffspechead\">API Diff Specification</td>");
	    h_.writeText("      </tr>");
	    h_.writeText("      <tr>");
	    h_.writeText("        <td class=\"diffspec\" style=\"padding-top:.25em\">To Level:</td>");
	    h_.writeText("        <td class=\"diffvaluenew\" style=\"padding-top:.25em\">" + newAPIName + "</td>");
	    h_.writeText("      </tr>");
	    h_.writeText("      <tr>");
	    h_.writeText("        <td class=\"diffspec\">From Level:</td>");
	    h_.writeText("        <td class=\"diffvalueold\">" + oldAPIName + "</td>");
	    h_.writeText("      </tr>");
	    h_.writeText("      <tr>");
	    h_.writeText("        <td class=\"diffspec\">Generated</td>");
	    h_.writeText("        <td class=\"diffvalue\">" + formatter.format( day ) + "</td>");
	    h_.writeText("      </tr>");
 	    h_.writeText("    </table>");
 	    h_.writeText("    </div><!-- End and-diff-id -->");

	    	h_.writeText("  <div class=\"and-diff-id\" style=\"margin-right:8px;\">");
	    	h_.writeText("    <table class=\"diffspectable\">");
	    	h_.writeText("      <tr>");
	    	h_.writeText("        <td class=\"diffspec\" colspan=\"2\"><a href=\"../changes.html\" target=\"_top\">Top of Report</a>");
	    	h_.writeText("      </tr>");
 	    	h_.writeText("    </table>");
	    	h_.writeText("  </div> <!-- End and-diff-id -->");

	    h_.writeText("  </div> <!-- End headerRight -->");
	    h_.writeText("  </div> <!-- End header -->");
	    h_.writeText("<div id=\"body-content\">");
	    h_.writeText("<div id=\"doc-content\" style=\"position:relative;\">");
	    h_.writeText("<div id=\"mainBodyFluid\">");

            h_.writeText("<h1>API&nbsp;Change&nbsp;Statistics</h1>");


            DecimalFormat df2 = new DecimalFormat( "#,###,###,##0.00" );
            double dd = apiDiff.pdiff;
            double dd2dec = new Double(df2.format(dd)).doubleValue();

            h_.writeText("<p>The overall difference between API Levels " + oldAPIName + " and " +  newAPIName + " is approximately <span style=\"color:222;font-weight:bold;\">" + dd2dec + "%</span>.");
            h_.writeText("</p>");

            h_.writeText("<br>");
            h_.writeText("<a name=\"numbers\"></a>");
            h_.writeText("<h2>Total of Differences, by Number and Type</h2>");
            h_.writeText("<p>");
            h_.writeText("The table below lists the numbers of program elements (packages, classes, constructors, methods, and fields) that were added, changed, or removed. The table includes only the highest-level program elements &mdash; that is, if a class with two methods was added, the number of methods added does not include those two methods, but the number of classes added does include that class.");
            h_.writeText("</p>");

            emitNumbersByElement(apiDiff);

            h_.writeText("<br>");
            h_.writeText("<a name=\"packages\"></a>");
            h_.writeText("<h2>Changed Packages, Sorted by Percentage Difference</h2>");
            emitPackagesByDiff(apiDiff);
            h_.writeText("<p style=\"font-size:10px\">* See <a href=\"#calculation\">Calculation of Change Percentages</a>, below.</p>");

            h_.writeText("<br>");
            h_.writeText("<a name=\"classes\"></a>");
            h_.writeText("<h2>Changed Classes and <i>Interfaces</i>, Sorted by Percentage Difference</h2>");
            emitClassesByDiff(apiDiff);
            h_.writeText("<p style=\"font-size:10px\">* See <a href=\"#calculation\">Calculation of Change Percentages</a>, below.</p>");

            h_.writeText("<br>");
            h_.writeText("<h2 id=\"calculation\">Calculation of Change Percentages</h2>");
            h_.writeText("<p>");
            h_.writeText("The percent change statistic reported for all elements in the &quot;to&quot; API Level specification is defined recursively as follows:</p>");
            h_.writeText("<pre>");
            h_.writeText("Percentage difference = 100 * (added + removed + 2*changed)");
            h_.writeText("                        -----------------------------------");
            h_.writeText("                        sum of public elements in BOTH APIs");
            h_.writeText("</pre>");
            h_.writeText("<p>where <code>added</code> is the number of packages added, <code>removed</code> is the number of packages removed, and <code>changed</code> is the number of packages changed.");
            h_.writeText("This definition is applied recursively for the classes and their program elements, so the value for a changed package will be less than 1, unless every class in that package has changed.");
            h_.writeText("The definition ensures that if all packages are removed and all new packages are");
            h_.writeText("added, the change will be 100%.</p>");

            h_.writeText("      </div>	");
            h_.writeText("      <div id=\"footer\">");
            h_.writeText("        <div id=\"copyright\">");
            h_.writeText("        Except as noted, this content is licensed under ");
            h_.writeText("        <a href=\"https://creativecommons.org/licenses/by/2.5/\"> Creative Commons Attribution 2.5</a>.");
            h_.writeText("        For details and restrictions, see the <a href=\"https://developer.android.com/license.html\">Content License</a>.");
            h_.writeText("        </div>");
            h_.writeText("      <div id=\"footerlinks\">");
            h_.writeText("      <p>");
            h_.writeText("        <a href=\"https://www.android.com/terms.html\">Site Terms of Service</a> -");
            h_.writeText("        <a href=\"https://www.android.com/privacy.html\">Privacy Policy</a> -");
            h_.writeText("        <a href=\"https://www.android.com/branding.html\">Brand Guidelines</a>");
            h_.writeText("      </p>");
            h_.writeText("    </div>");
            h_.writeText("    </div> <!-- end footer -->");
            h_.writeText("    </div><!-- end doc-content -->");
            h_.writeText("    </div> <!-- end body-content --> ");

            h_.writeText("<script src=\"https://www.google-analytics.com/ga.js\" type=\"text/javascript\">");
            h_.writeText("</script>");
            h_.writeText("<script type=\"text/javascript\">");
            h_.writeText("  try {");
            h_.writeText("    var pageTracker = _gat._getTracker(\"UA-5831155-1\");");
            h_.writeText("    pageTracker._setAllowAnchor(true);");
            h_.writeText("    pageTracker._initData();");
            h_.writeText("    pageTracker._trackPageview();");
            h_.writeText("  } catch(e) {}");
            h_.writeText("</script>");

            h_.writeText("</BODY></HTML>");
            h_.reportFile.close();
        } catch(IOException e) {
            System.out.println("IO Error while attempting to create " + filename);
            System.out.println("Error: " + e.getMessage());
            System.exit(1);
        }
    }

    /**
     * Emit all packages sorted by percentage difference, and a histogram
     * of the values.
     */
    public void emitPackagesByDiff(APIDiff apiDiff) {

        Collections.sort(apiDiff.packagesChanged, new ComparePkgPdiffs());

        // Write out the table start
        h_.writeText("<TABLE summary=\"Packages sorted by percentage difference\" WIDTH=\"100%\">");
        h_.writeText("<TR>");
        h_.writeText("  <TH  WIDTH=\"10%\">Percentage Difference*</TH>");
        h_.writeText("  <TH>Package</TH>");
        h_.writeText("</TR>");

        int[] hist = new int[101];
        for (int i = 0; i < 101; i++) {
            hist[i] = 0;
        }

        Iterator iter = apiDiff.packagesChanged.iterator();
        while (iter.hasNext()) {
            PackageDiff pkg = (PackageDiff)(iter.next());
            int bucket = (int)(pkg.pdiff);
            hist[bucket]++;
            h_.writeText("<TR>");
            if (bucket != 0)
                h_.writeText("  <TD ALIGN=\"center\">" + bucket + "</TD>");
            else
                h_.writeText("  <TD ALIGN=\"center\">&lt;1</TD>");
            h_.writeText("  <TD><A HREF=\"pkg_" + pkg.name_ + h_.reportFileExt + "\">" + pkg.name_ + "</A></TD>");
            h_.writeText("</TR>");
        }

        h_.writeText("</TABLE>");
        
        /* Emit the histogram of the results
        h_.writeText("<hr>");
        h_.writeText("<p><a name=\"packages_hist\"></a>");
        h_.writeText("<TABLE summary=\"Histogram of the package percentage differences\" BORDER=\"1\" cellspacing=\"0\" cellpadding=\"0\">");
        h_.writeText("<TR>");
        h_.writeText("  <TD ALIGN=\"center\" bgcolor=\"#EEEEFF\"><FONT size=\"+1\"><b>Percentage<br>Difference</b></FONT></TD>");
        h_.writeText("  <TD ALIGN=\"center\" bgcolor=\"#EEEEFF\"><FONT size=\"+1\"><b>Frequency</b></FONT></TD>");
        h_.writeText("  <TD width=\"300\" ALIGN=\"center\" bgcolor=\"#EEEEFF\"><FONT size=\"+1\"><b>Percentage Frequency</b></FONT></TD>");
        h_.writeText("</TR>");

        double total = 0;
        for (int i = 0; i < 101; i++) {
            total += hist[i];
        }
        for (int i = 0; i < 101; i++) {
            if (hist[i] != 0) {
                h_.writeText("<TR>");
                h_.writeText("  <TD ALIGN=\"center\">" + i + "</TD>");
                h_.writeText("  <TD>" + (hist[i]/total) + "</TD>");
                h_.writeText("  <TD><img alt=\"|\" src=\"../black.gif\" height=20 width=" + (hist[i]*300/total) + "></TD>");
                h_.writeText("</TR>");
            }
        }
        // Repeat the data in a format which is easier for spreadsheets
        h_.writeText("<!-- START_PACKAGE_HISTOGRAM");
        for (int i = 0; i < 101; i++) {
            if (hist[i] != 0) {
                h_.writeText(i + "," + (hist[i]/total));
            }
        }
        h_.writeText("END_PACKAGE_HISTOGRAM -->");
        
        h_.writeText("</TABLE>");
	*/
    }

    /**
     * Emit all classes sorted by percentage difference, and a histogram
     * of the values..
     */
    public void emitClassesByDiff(APIDiff apiDiff) {
        // Add all the changed classes to a list
        List allChangedClasses = new ArrayList();
        Iterator iter = apiDiff.packagesChanged.iterator();
        while (iter.hasNext()) {
            PackageDiff pkg = (PackageDiff)(iter.next());
            if (pkg.classesChanged != null) {
                // Add the package name to the class name
                List cc = new ArrayList(pkg.classesChanged);
                Iterator iter2 = cc.iterator();
                while (iter2.hasNext()) {
                    ClassDiff classDiff = (ClassDiff)(iter2.next());
                    classDiff.name_ = pkg.name_ + "." + classDiff.name_;
                }
                allChangedClasses.addAll(cc);
            }
        }
        Collections.sort(allChangedClasses, new CompareClassPdiffs());

        // Write out the table start
        h_.writeText("<TABLE summary=\"Classes sorted by percentage difference\" WIDTH=\"100%\">");
        h_.writeText("<TR WIDTH=\"20%\">");
        h_.writeText("  <TH WIDTH=\"10%\">Percentage<br>Difference*</TH>");
        h_.writeText("  <TH><b>Class or <i>Interface</i></b></TH>");
        h_.writeText("</TR>");

        int[] hist = new int[101];
        for (int i = 0; i < 101; i++) {
            hist[i] = 0;
        }

        iter = allChangedClasses.iterator();
        while (iter.hasNext()) {
            ClassDiff classDiff = (ClassDiff)(iter.next());
            int bucket = (int)(classDiff.pdiff);
            hist[bucket]++;
            h_.writeText("<TR>");
            if (bucket != 0)
                h_.writeText("  <TD ALIGN=\"center\">" + bucket + "</TD>");
            else
                h_.writeText("  <TD ALIGN=\"center\">&lt;1</TD>");
            h_.writeText("  <TD><A HREF=\"" + classDiff.name_ + h_.reportFileExt + "\">");
            if (classDiff.isInterface_)
                h_.writeText("<i>" + classDiff.name_ + "</i></A></TD>");
            else
                h_.writeText(classDiff.name_ + "</A></TD>");
            h_.writeText("</TR>");
        }

        h_.writeText("</TABLE>");

        /* Emit the histogram of the results
        h_.writeText("<hr>");
        h_.writeText("<p><a name=\"classes_hist\"></a>");
        h_.writeText("<TABLE summary=\"Histogram of the class percentage differences\" BORDER=\"1\" cellspacing=\"0\" cellpadding=\"0\">");
        h_.writeText("<TR>");
        h_.writeText("  <TD ALIGN=\"center\" bgcolor=\"#EEEEFF\"><FONT size=\"+1\"><b>Percentage<br>Difference</b></FONT></TD>");
        h_.writeText("  <TD ALIGN=\"center\" bgcolor=\"#EEEEFF\"><FONT size=\"+1\"><b>Frequency</b></FONT></TD>");
        h_.writeText("  <TD width=\"300\" ALIGN=\"center\" bgcolor=\"#EEEEFF\"><FONT size=\"+1\"><b>Percentage Frequency</b></FONT></TD>");
        h_.writeText("</TR>");

        double total = 0;
        for (int i = 0; i < 101; i++) {
            total += hist[i];
        }
        for (int i = 0; i < 101; i++) {
            if (hist[i] != 0) {
                h_.writeText("<TR>");
                h_.writeText("  <TD ALIGN=\"center\">" + i + "</TD>");
                h_.writeText("  <TD>" + (hist[i]/total) + "</TD>");
                h_.writeText("  <TD><img alt=\"|\" src=\"../black.gif\" height=20 width=" + (hist[i]*300/total) + "></TD>");
                h_.writeText("</TR>");
            }
        }
        // Repeat the data in a format which is easier for spreadsheets
        h_.writeText("<!-- START_CLASS_HISTOGRAM");
        for (int i = 0; i < 101; i++) {
            if (hist[i] != 0) {
                h_.writeText(i + "," + (hist[i]/total));
            }
        }
        h_.writeText("END_CLASS_HISTOGRAM -->");
        
        h_.writeText("</TABLE>");
	*/
    }

    /**
     * Emit a table of numbers of removals, additions and changes by
     * package, class, constructor, method and field.
     */
    public void emitNumbersByElement(APIDiff apiDiff) {

        // Local variables to hold the values
        int numPackagesRemoved = apiDiff.packagesRemoved.size();
        int numPackagesAdded = apiDiff.packagesAdded.size();
        int numPackagesChanged = apiDiff.packagesChanged.size();

        int numClassesRemoved = 0;
        int numClassesAdded = 0;
        int numClassesChanged = 0;

        int numCtorsRemoved = 0;
        int numCtorsAdded = 0;
        int numCtorsChanged = 0;

        int numMethodsRemoved = 0;
        int numMethodsAdded = 0;
        int numMethodsChanged = 0;

        int numFieldsRemoved = 0;
        int numFieldsAdded = 0;
        int numFieldsChanged = 0;

        int numRemoved = 0;
        int numAdded = 0;
        int numChanged = 0;

        // Calculate the values
        Iterator iter = apiDiff.packagesChanged.iterator();
        while (iter.hasNext()) {
            PackageDiff pkg = (PackageDiff)(iter.next());
            numClassesRemoved += pkg.classesRemoved.size();
            numClassesAdded += pkg.classesAdded.size();
            numClassesChanged += pkg.classesChanged.size();

            Iterator iter2 = pkg.classesChanged.iterator();
            while (iter2.hasNext()) {
                 ClassDiff classDiff = (ClassDiff)(iter2.next());
                 numCtorsRemoved += classDiff.ctorsRemoved.size();
                 numCtorsAdded += classDiff.ctorsAdded.size();
                 numCtorsChanged += classDiff.ctorsChanged.size();
                 
                 numMethodsRemoved += classDiff.methodsRemoved.size();
                 numMethodsAdded += classDiff.methodsAdded.size();
                 numMethodsChanged += classDiff.methodsChanged.size();

                 numFieldsRemoved += classDiff.fieldsRemoved.size();
                 numFieldsAdded += classDiff.fieldsAdded.size();
                 numFieldsChanged += classDiff.fieldsChanged.size();
            }
        }

        // Write out the table
        h_.writeText("<TABLE summary=\"Number of differences\" WIDTH=\"100%\">");
        h_.writeText("<TR>");
        h_.writeText("  <th>Type</th>");
        h_.writeText("  <TH ALIGN=\"center\"><b>Additions</b></TH>");
        h_.writeText("  <TH ALIGN=\"center\"><b>Changes</b></TH>");
        h_.writeText("  <TH ALIGN=\"center\">Removals</TH>");
        h_.writeText("  <TH ALIGN=\"center\"><b>Total</b></TH>");
        h_.writeText("</TR>");

        h_.writeText("<TR>");
        h_.writeText("  <TD>Packages</TD>");

        h_.writeText("  <TD ALIGN=\"right\">" + numPackagesAdded + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numPackagesChanged + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numPackagesRemoved + "</TD>");
        int numPackages = numPackagesRemoved + numPackagesAdded + numPackagesChanged;
        h_.writeText("  <TD ALIGN=\"right\">" + numPackages + "</TD>");
        h_.writeText("</TR>");

        numRemoved += numPackagesRemoved;
        numAdded += numPackagesAdded;
        numChanged += numPackagesChanged;

        h_.writeText("<TR>");
        h_.writeText("  <TD>Classes and <i>Interfaces</i></TD>");

        h_.writeText("  <TD ALIGN=\"right\">" + numClassesAdded + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numClassesChanged + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numClassesRemoved + "</TD>");
        int numClasses = numClassesRemoved + numClassesAdded + numClassesChanged;
        h_.writeText("  <TD ALIGN=\"right\">" + numClasses + "</TD>");
        h_.writeText("</TR>");

        numRemoved += numClassesRemoved;
        numAdded += numClassesAdded;
        numChanged += numClassesChanged;

        h_.writeText("<TR>");
        h_.writeText("  <TD>Constructors</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numCtorsAdded + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numCtorsChanged + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numCtorsRemoved + "</TD>");
        int numCtors = numCtorsRemoved + numCtorsAdded + numCtorsChanged;
        h_.writeText("  <TD ALIGN=\"right\">" + numCtors + "</TD>");
        h_.writeText("</TR>");

        numRemoved += numCtorsRemoved;
        numAdded += numCtorsAdded;
        numChanged += numCtorsChanged;

        h_.writeText("<TR>");
        h_.writeText("  <TD>Methods</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numMethodsAdded + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numMethodsChanged + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numMethodsRemoved + "</TD>");
        int numMethods = numMethodsRemoved + numMethodsAdded + numMethodsChanged;
        h_.writeText("  <TD ALIGN=\"right\">" + numMethods + "</TD>");
        h_.writeText("</TR>");

        numRemoved += numMethodsRemoved;
        numAdded += numMethodsAdded;
        numChanged += numMethodsChanged;

        h_.writeText("<TR>");
        h_.writeText("  <TD>Fields</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numFieldsAdded + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numFieldsChanged + "</TD>");
        h_.writeText("  <TD ALIGN=\"right\">" + numFieldsRemoved + "</TD>");
        int numFields = numFieldsRemoved + numFieldsAdded + numFieldsChanged;
        h_.writeText("  <TD ALIGN=\"right\">" + numFields + "</TD>");
        h_.writeText("</TR>");

        numRemoved += numFieldsRemoved;
        numAdded += numFieldsAdded;
        numChanged += numFieldsChanged;

        h_.writeText("<TR>");
        h_.writeText("  <TD style=\"background-color:#FAFAFA\"><b>Total</b></TD>");

        h_.writeText("  <TD  style=\"background-color:#FAFAFA\" ALIGN=\"right\"><strong>" + numAdded + "</strong></TD>");
        h_.writeText("  <TD  style=\"background-color:#FAFAFA\" ALIGN=\"right\"><strong>" + numChanged + "</strong></TD>");
        h_.writeText("  <TD  style=\"background-color:#FAFAFA\" ALIGN=\"right\"><strong>" + numRemoved + "</strong></TD>");
        int total = numRemoved + numAdded + numChanged;
        h_.writeText("  <TD  style=\"background-color:#FAFAFA\" ALIGN=\"right\"><strong>" + total + "</strong></TD>");
        h_.writeText("</TR>");

        h_.writeText("</TABLE>");
    }

}

