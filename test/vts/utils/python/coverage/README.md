# To run from another Python module

Import the CoverageReport module by including the line:

   from vts.utils.python.coverage import CoverageReport

Run the code by calling the parse function as follows:
   html_report = CoverageReport.GenerateCoverageReport(src_file_name, src_file_content, gcov_file_content,
                           gcda_file_content)

Args:
    src_file_name: string, the source file name.
    src_file_content: string, the C/C++ source file content.
    gcov_file_content: string, the raw gcov binary file content.
    gcda_file_content: string, the raw gcda binary file content.

Returns:
    the coverage HTML produced for 'src_file_name'.
