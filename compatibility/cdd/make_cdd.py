#!/usr/bin/python
"""
Utility for building the CDD from component markdown files.

From the compatibility/cdd directory, run:
python make-cdd.py --version <version number> --branch <AOSP branch>
    --output <output file name>

Each generated CDD file is marked with a hash based on the content of the input files.

TODO(gdimino): Clean up and comment this code.
"""

from bs4 import BeautifulSoup
import argparse
import codecs
import jinja2
import markdown
import os
import re
import subprocess
import tidylib


HEADERS_FOR_TOC = ['h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7']
ANDROID_VERSION = "7.0, (N)"
TOC_PER_COL = 34

def get_section_info(my_path):
  # (_, _, filenames) = os.walk(my_path).next()
  section_info = [];
  # Get section info from every file whose name contains a number. TODO: fix
  # this ugly hack.
  # for rootdir, subdirs, files in os.walk(my_path):
  for dir in get_immediate_subdirs(my_path):
    # for dir  in subdirs:
    if (not dir.isalpha() and dir != 'older-versions' and dir != '.git'):
      child_data = []
      print 'dir = ' + dir
      for file in os.listdir(dir):
        if '.md' in file:
          if file == 'index.md':
            number =  0
          else:
            number = int((file.split('_')[1]))
          print 'file = ' + file + ', dir = ' + dir
          html_string = markdown.markdown(codecs.open(my_path + '/' + dir + '/' + file, 'r', encoding='utf-8').read())
          child_data.append({'file': file,
                             'number': number,
                             'title': dir.split('_')[-1],
                             'html': html_string,
                             'children':[]})
      child_data.sort(key=lambda child: child['number'])
      section_info.append({'id': dir,
                           'number': int(''.join((dir.split('_')[:-1])).replace("_", ".")),
                           'title': dir.split('_')[-1],
                           'html': '',
                           'children':child_data})
  section_info.sort(key=lambda section: section['number'])
  return section_info


def get_soup(section_info):
  html_body_text = u'''<!DOCTYPE html>
<head>
<title>Android ''' + ANDROID_VERSION + ''' Compatibility Definition</title>
<link rel="stylesheet" type="text/css" href="source/android-cdd.css"/>
<meta charset="utf-8" />
</head>
<body>
<div id="main">'''

  for section in section_info:
     for child in section['children']:
       html_body_text += child['html']
  html_body_text +=  '</div></body><html>'
  return BeautifulSoup(html_body_text)

def get_soup_devsite(section_info):
  html_body_text = ''
  for section in section_info:
     for child in section['children']:
       html_body_text += child['html']
  return BeautifulSoup(html_body_text)


def add_id_to_section_headers(soup):
  header_tags = ['h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7']
  for tag in soup.find_all(header_tags):
    tag['id'] = create_id(tag)

def generate_toc(soup):
  toc_html = '<div id="toc">'
  header_tags = ['h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7']
  toc_entries =  soup.find_all(header_tags)
  toc_chunks = [toc_entries[i:i + TOC_PER_COL] for i in xrange(0, len(toc_entries), TOC_PER_COL)]
  print 'Number of chunks =  %d' % len(toc_chunks)
  for chunk in toc_chunks:
    if not toc_chunks.index(chunk) %2:
      toc_html = toc_html + ('<div id="toc_left">')
      for tag in chunk:
        toc_html = toc_html + '<p class="toc_' + tag.name + '"><a href= "#' + create_id(tag) + '">' + tag.contents[0] + '</a></p>'
      toc_html = toc_html + ('</div>')
    else:
      toc_html = toc_html + ('<div id="toc_right">')
      for tag in chunk:
        toc_html = toc_html + '<p class="toc_' + tag.name + '"><a href= "#' + create_id(tag) + '">' + tag.contents[0] + '</a></p>'
      toc_html = toc_html + ('</div>')
      toc_html = toc_html + '<div style="clear: both; page-break-after:always; height:1px"></div>'
  toc_html = toc_html + '<div style="clear: both"></div>'
  return (BeautifulSoup(toc_html).body.contents)

def add_toc(soup):
  toc_contents = generate_toc(soup)[0]
  toc_title =  BeautifulSoup("<h6>Table of Contents</h6>").body.contents[0]
  soup.body.insert(0, toc_contents)
  soup.body.insert(0, toc_title)
  return soup

def create_id(header_tag):
  return header_tag.contents[0].lower().replace('. ', '_').replace(' ', '_').replace('.', '_')

def decrease_headings(soup):
  heading_tags = ['h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7', 'h8']
  headings =  soup.find_all(heading_tags)
  for heading in headings:
    level = int(re.search(r'(\d)', heading.name).groups()[0])
    heading.name = 'h%d' % (level + 1)
  return soup

def get_version_branch_and_output():
  # Get command-line args.  If there aren't any, then prompt for user input.
  parser = argparse.ArgumentParser()
  parser.add_argument('--version', help='Android version')
  parser.add_argument('--branch', help='AOSP branch')
  parser.add_argument('--output', help='Base name of output file')
  args = parser.parse_args()

  if not args.version:
    args.version = raw_input('Android version for CDD: ')
  if not args.branch:
    args.branch = raw_input('Current AOSP branch for changelog: ')
  if not args.output:
    args.output = raw_input('Base name of desired output file: ')
  return (args.version, args.branch, args.output)

# Utilities
def get_immediate_subdirs(dir):
    return [name for name in os.listdir(dir)
            if os.path.isdir(os.path.join(dir, name))]

def render_content(page_info, template_filename):
  fp = open(template_filename)
  temp_file = fp.read().encode('utf8')
  fp.close()
  return jinja2.Template(temp_file).render(page_info)

# Odds and ends

def check_section_numbering(soup):
  header_tags = ['h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'h7']
  for tag in header_tags:
    headings = soup.find_all(tag)
    header_numbers = []
    for heading in headings:
      header_numbers.append(re.sub(r"([\d.]*).*", r"\1"), heading.contents)
  return true

# Abandoned in favor of tidy.
def elim_para_whitespace(html):
  new_html = re.sub(re.compile(r"(<p[^>]*>)\s*\n\s*(<a[^>]*>)\n([^<]*)\n\s*(</a>)\n\s*(</p>)", re.M),r"\1\2\3\4\5\n", html)
  return new_html


def elim_space_before_punc(html):
  new_html = re.sub(re.compile(r"</a>\s+([.,;:])", re.M),r"</a>\1", html)
  return new_html


def main():
  # Read version and branch info and output file name.
  (ANDROID_VERSION, CURRENT_BRANCH, output_filename) = get_version_branch_and_output()

  # Scan current directory for source files and compile info for the toc..
  my_path = os.getcwd()
  section_info = get_section_info(my_path)

  # Get page info
  page_info = { 'title': 'Android ANDROID_VERSION Compatibility Definition',
                'book_path': '/_book.yaml',
                'project_path': '/_project.yaml'
               }

  # Generate the HTML for PDF
  soup = get_soup(section_info)
  add_id_to_section_headers(soup)
  add_toc(soup)
  html = soup.prettify(formatter='html')

  # Generate the HTML for devsite
  devsite_soup = get_soup_devsite(section_info)
  add_id_to_section_headers(devsite_soup)
  add_id_to_section_headers(soup)
  page_info['body_html'] =  decrease_headings(devsite_soup)
  devsite_html = render_content(page_info, 'source/devsite_template.html')

  html = soup.prettify(formatter='html')

  # Add version and branch info
  html = re.sub(re.compile(r'ANDROID_VERSION'), ANDROID_VERSION, html)
  html = re.sub(re.compile(r'CURRENT_BRANCH'), CURRENT_BRANCH, html)

  devsite_html = re.sub(re.compile(r'ANDROID_VERSION'), ANDROID_VERSION, devsite_html)
  devsite_html = re.sub(re.compile(r'CURRENT_BRANCH'), CURRENT_BRANCH, devsite_html)

  # Apply HTML Tidy to output
  (document, errors) = tidylib.tidy_document(html, options={'doctype': 'omit'})
  (devsite_document, errors) = tidylib.tidy_document(devsite_html, options={'doctype': 'omit'})

  # Eliminate space before punctuation
  html = elim_space_before_punc(html)
  devsite_html = elim_space_before_punc(devsite_html)

  # Write output files
  output = codecs.open('%s.html' % output_filename, 'w', encoding='utf-8')
  output.write(document)
  output.close()

  devsite_output = codecs.open('%s-devsite.html' % output_filename, 'w', encoding='utf-8')
  devsite_output.write(devsite_document)
  output.close()

  # Code to generate PDF
  # TODO(gdimino)

  # subprocess.call('wkhtmltopdf -B 1in -T 1in -L .75in -R .75in page ' +  
  #                output_filename +  
  #                ' --footer-html source/android-cdd-footer.html  /tmp/android-cdd-body.pdf', shell=True)

if __name__ == '__main__':
  main()




