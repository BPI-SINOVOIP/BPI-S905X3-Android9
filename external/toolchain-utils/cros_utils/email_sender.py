#!/usr/bin/env python2

# Copyright 2011 Google Inc. All Rights Reserved.
"""Utilities to send email either through SMTP or SendGMR."""

from __future__ import print_function

from email import encoders as Encoders
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import os
import smtplib
import tempfile

from cros_utils import command_executer


class EmailSender(object):
  """Utility class to send email through SMTP or SendGMR."""

  class Attachment(object):
    """Small class to keep track of attachment info."""

    def __init__(self, name, content):
      self.name = name
      self.content = content

  def SendEmail(self,
                email_to,
                subject,
                text_to_send,
                email_cc=None,
                email_bcc=None,
                email_from=None,
                msg_type='plain',
                attachments=None):
    """Choose appropriate email method and call it."""
    if os.path.exists('/usr/bin/sendgmr'):
      self.SendGMREmail(email_to, subject, text_to_send, email_cc, email_bcc,
                        email_from, msg_type, attachments)
    else:
      self.SendSMTPEmail(email_to, subject, text_to_send, email_cc, email_bcc,
                         email_from, msg_type, attachments)

  def SendSMTPEmail(self, email_to, subject, text_to_send, email_cc, email_bcc,
                    email_from, msg_type, attachments):
    """Send email via standard smtp mail."""
    # Email summary to the current user.
    msg = MIMEMultipart()

    if not email_from:
      email_from = os.path.basename(__file__)

    msg['To'] = ','.join(email_to)
    msg['Subject'] = subject

    if email_from:
      msg['From'] = email_from
    if email_cc:
      msg['CC'] = ','.join(email_cc)
      email_to += email_cc
    if email_bcc:
      msg['BCC'] = ','.join(email_bcc)
      email_to += email_bcc

    msg.attach(MIMEText(text_to_send, msg_type))
    if attachments:
      for attachment in attachments:
        part = MIMEBase('application', 'octet-stream')
        part.set_payload(attachment.content)
        Encoders.encode_base64(part)
        part.add_header('Content-Disposition',
                        "attachment; filename=\"%s\"" % attachment.name)
        msg.attach(part)

    # Send the message via our own SMTP server, but don't include the
    # envelope header.
    s = smtplib.SMTP('localhost')
    s.sendmail(email_from, email_to, msg.as_string())
    s.quit()

  def SendGMREmail(self, email_to, subject, text_to_send, email_cc, email_bcc,
                   email_from, msg_type, attachments):
    """Send email via sendgmr program."""
    ce = command_executer.GetCommandExecuter(log_level='none')

    if not email_from:
      email_from = os.path.basename(__file__)

    to_list = ','.join(email_to)

    if not text_to_send:
      text_to_send = 'Empty message body.'
    body_fd, body_filename = tempfile.mkstemp()
    to_be_deleted = [body_filename]

    try:
      os.write(body_fd, text_to_send)
      os.close(body_fd)

      # Fix single-quotes inside the subject. In bash, to escape a single quote
      # (e.g 'don't') you need to replace it with '\'' (e.g. 'don'\''t'). To
      # make Python read the backslash as a backslash rather than an escape
      # character, you need to double it. So...
      subject = subject.replace("'", "'\\''")

      if msg_type == 'html':
        command = ("sendgmr --to='%s' --subject='%s' --html_file='%s' "
                   '--body_file=/dev/null' % (to_list, subject, body_filename))
      else:
        command = ("sendgmr --to='%s' --subject='%s' --body_file='%s'" %
                   (to_list, subject, body_filename))
      if email_from:
        command += ' --from=%s' % email_from
      if email_cc:
        cc_list = ','.join(email_cc)
        command += " --cc='%s'" % cc_list
      if email_bcc:
        bcc_list = ','.join(email_bcc)
        command += " --bcc='%s'" % bcc_list

      if attachments:
        attachment_files = []
        for attachment in attachments:
          if '<html>' in attachment.content:
            report_suffix = '_report.html'
          else:
            report_suffix = '_report.txt'
          fd, fname = tempfile.mkstemp(suffix=report_suffix)
          os.write(fd, attachment.content)
          os.close(fd)
          attachment_files.append(fname)
        files = ','.join(attachment_files)
        command += " --attachment_files='%s'" % files
        to_be_deleted += attachment_files

      # Send the message via our own GMR server.
      status = ce.RunCommand(command)
      return status

    finally:
      for f in to_be_deleted:
        os.remove(f)
