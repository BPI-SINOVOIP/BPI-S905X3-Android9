#!/usr/bin/env python
#
# Copyright (C) 2018 Amlogic, Inc. All rights reserved.
#
# All information contained herein is Amlogic confidential.
#
# This software is provided to you pursuant to Software License
# Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
# used only in accordance with the terms of this agreement.
#
# Redistribution and use in source and binary forms, with or without
# modification is strictly prohibited without prior written permission
# from Amlogic.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

def get_args():
	from argparse import ArgumentParser

	parser = ArgumentParser()
	parser.add_argument('--in', required=True, dest='inf', help='Name of input file')
	parser.add_argument('--out', type=str, default='null', help='key output dir')
	parser.add_argument('--skip', type=int, default=1, help='key cutting interval')

	return parser.parse_args()

def file_write(head, start_p, length, fd, name):
	import gc
	ret = 0
	size_h = len(head)
	fd1 = open(name, 'w+')

	fd1.write(head)
	fd.seek(start_p, 0)
	buf = fd.read(length)
	fd1.write(buf)

	del buf
	gc.collect()
	fd1.close()

	return ret

def file_get_pos(start, end, fd, cursor):
	import re
	import gc
	ret = 0

	fd.seek(cursor, 0)
# 16KB buf > a attestation key size
	keybuf = fd.read(0x4000)

	start_m = re.search(start, keybuf)
	if start_m == None:
		ret = -1
		print "not found the start position of key\n"
		return ret, 0, 0

	end_m = re.search(end, keybuf)
	if end_m == None:
		ret = -1
		return ret, 0, 0

	start_p = cursor + start_m.end()
	end_p = cursor + end_m.end()

	del keybuf
	gc.collect()
	return ret, start_p, end_p

def key_pack(dirname, name_prefix):
	import sys
	import os
	import struct

	keyname = dirname + '/' + name_prefix + ".keybox"

	keyoffset = [0] * 8
	keysize = [0] * 8
	keynum = 0
# version,keynum,keyoffset[],keysize[] = 18int
	keyheadlen = struct.calcsize('<IIIIIIIIIIIIIIIIII')
	keyoffset[0] = keyheadlen

	file_name = ("AttestKey.ec", "AttestCert.ec0", "AttestCert.ec1", "AttestCert.ec2", \
			"AttestKey.rsa", "AttestCert.rsa0", "AttestCert.rsa1", "AttestCert.rsa2", )

	for i in range(0, 8):
		if os.path.exists(dirname + "/" + file_name[i]):
			fd = open(dirname + "/" + file_name[i], 'rb')
			fd.seek(0, 2)
			keysize[i] = fd.tell()
			keynum = keynum + 1
			fd.close()
		else:
			keysize[i] = 0

		if i != 0:
			keyoffset[i] = keyoffset[i-1] + keysize[i-1]

	keyhead = struct.pack('<IIIIIIIIIIIIIIIIII', \
			0x2, keynum, keyoffset[0], \
			keysize[0], keyoffset[1], keysize[1], keyoffset[2], keysize[2], \
			keyoffset[3], keysize[3], keyoffset[4], keysize[4], \
			keyoffset[5], keysize[5], keyoffset[6], keysize[6], \
			keyoffset[7], keysize[7])

	fdw = open(keyname, 'w+')
	fdw.seek(0, 0)
	fdw.write(keyhead)

	for i in range(0, 8):
		if keysize[i] != 0:
			fd = open(dirname + "/" + file_name[i], 'rb')
			buf = fd.read()
			fdw.write(buf)
			fd.close()
			os.system("rm -f " + dirname + "/" + file_name[i])

	fdw.close()

def key_partial_cut(data_file, suffix):

	import sys
	import os

	cert_s = "-----BEGIN CERTIFICATE-----"
	cert_e = "-----END CERTIFICATE-----"
	if suffix == "ec":
		key_s = "-----BEGIN EC PRIVATE KEY-----"
		key_e = "-----END EC PRIVATE KEY-----"
	else:
		key_s = "-----BEGIN RSA PRIVATE KEY-----"
		key_e = "-----END RSA PRIVATE KEY-----"

	dirname = os.path.dirname(data_file) + "/"
	fd = open(data_file, 'rb')

	dername = dirname + "AttestKey." + suffix
	pemname = dirname + "AttestKey." + suffix + ".pem"
	(ret, start_p, end_p) = file_get_pos(key_s, key_e, fd, 0)
	if ret != 0:
		print 'cannot find the specified string %s or %s after the seek of the file' %(key_s, key_e)
		fd.close()
		sys.exit(0)


	ret = file_write(key_s, start_p, end_p - start_p, fd, pemname)
	if ret != 0:
		print 'write file %s failed' %pemname
		fd.close()
		sys.exit(0)
	cmd = "openssl " + suffix + " -inform PEM -in " + pemname + " -outform DER -out " + dername
	os.system(cmd)
	os.system("rm -f " + pemname)

	for i in range(0, 3):
		(ret, start_p, end_p) = file_get_pos(cert_s, cert_e, fd, end_p)
		if ret != 0:
			print 'cannot find the specified string %s or %s after the seek of the file' %(cert_s, cert_e)
			sys.exit(0)
		dername = dirname + "AttestCert." + suffix + bytes(i)
		pemname = dirname + "AttestCert." + suffix + bytes(i) + ".pem"
		ret = file_write(cert_s, start_p, end_p - start_p, fd, pemname)
		os.system("openssl x509 -inform PEM -in " + pemname + " -outform DER -out " + dername)
		os.system("rm -f " + pemname)
		if ret != 0:
			print 'write file %s failed' %pemname
			sys.exit(0)

	fd.close()

	return ret

def key_file_cut(data_file, dirname):

	import sys
	import os
	import gc

	key_s = "<Keybox DeviceID"
	key_e = "</Key></Keybox>"
	args = get_args()
	skip_num = args.skip

	fd = open(data_file, 'rb')

	# find the end of  the file and redundant 16 Bytes
	fd.seek(0,2)
	file_end = fd.tell() - len("</AndroidAttestation>") - 16;

	end_p = 0;
	key_num = 0;
	while end_p < file_end:
		(ret, start_p, end_p) = file_get_pos(key_s, key_e, fd, end_p)
		if ret != 0:
			print 'cannot find the specified string %s or %s after the seek of the file' %(key_s, key_e)
			fd.close()
			sys.exit(0)
		if key_num % skip_num != 0:
			# Do not skip the last key
			if end_p + 4096 < file_end:
				key_num += 1;
				continue

		key_num += 1;
		(ret, name_sp, name_ep) = file_get_pos('="', '">', fd, start_p)
		if ret != 0:
			print 'cannot find the specified string = or > after the seek of the file'
			fd.close()
			sys.exit(0)

		fd.seek(name_sp, 0)
		name_prefix = fd.read(name_ep - name_sp - 2)
		file_name = dirname + "/" + name_prefix + ".origin";

		file_write(key_s, start_p, end_p - start_p, fd, file_name)
		key_partial_cut(file_name, "ec")
		key_partial_cut(file_name, "rsa")
		os.system("rm -f " + file_name)
		key_pack(dirname, name_prefix)

		gc.collect()

	fd.close()

	return ret

def main():
	import os
	import sys

	# parse arguments
	args = get_args()

	if args.out != 'null':
		dirname = args.out
		if os.path.exists(dirname) == False:
			os.mkdir(dirname)
	else:
		dirname = "./"

	ret = key_file_cut(args.inf, dirname);

if __name__ == "__main__":
	main()
