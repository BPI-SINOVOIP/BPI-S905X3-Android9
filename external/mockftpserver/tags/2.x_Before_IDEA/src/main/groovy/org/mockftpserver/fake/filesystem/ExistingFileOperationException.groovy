/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.fake.filesystem

/**
 * Exception thrown when a requested operation is not allowed or possible on a directory 
 * or file that should already exist. This type of error typically causes a 550 reply 
 * code from an FTP server. Causes include:
 * <ul>
 *   <li>The file/directory does not exist</li>
 *   <li>The existing file/directory does not meet some required condition. e.g., trying to remove a non-empty directory</li>
 *   <li>The specified path is expected to be a file, but actually specifies an existing directory, or vice versa</li>
 * </ul>
 */
class ExistingFileOperationException extends FileSystemException {

     String path
     
     /**
      * @param path
      */
      ExistingFileOperationException(String path) {
         super(msg(path))
         this.path = path
     }

     /**
      * @param path
      * @param cause
      */
      ExistingFileOperationException(Throwable cause, String path) {
         super(msg(path), cause)
         this.path = path
     }

      private static String msg(path) {
          "Error occurred for [$path]"
      }
}