
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
from google import protobuf
from gcloud import bigtable

_COLUMN_FAMILY_ID = 'cf1'


class BigTableClient(object):
    """Defines the big table client that connects to the big table.

    Attributes:
        _column_family_id: A String for family of columns.
        _client: An instance of Client which is project specific.
        _client_instance: Representation of a Google Cloud Bigtable Instance.
        _start_index: Start index for the row key. It gets incremented as we
            dequeue.
        _end_index : End index for row key. This is incremented as we Enqueue.
        _table_name: A string that represents the big table.
        _table_instance: An instance of the Table that represents the big table.
    """

    def __init__(self, table, project_id):
        self._column_family_id = _COLUMN_FAMILY_ID
        self._client = bigtable.Client(project=project_id, admin=True)
        self._client_instance = None
        self._start_index = 0
        self._end_index = 0
        self._table_name = table
        self._table_instance = None
        # Start client to enable receiving requests
        self.StartClient()

    def StartClient(self, instance_id):
        """Starts client to prepare it to make requests."""

        # Start the client
        if not self._client.is_started():
            self._client.start()
        self._client_instance = self._client.instance(instance_id)
        if self._table_instance is None:
            self._table_instance = self._client_instance.table(self._table_name)

    def StopClient(self):
        """Stop client to close all the open gRPC clients."""

        # stop client
        self._client.stop()

    def CreateTable(self):
        """Creates a table in which read/write operations are performed.

        Raises:
            AbortionError: Error occurred when creating table is not successful.
                This could be due to creating a table with a duplicate name.
        """

        # Create a table
        logging.debug('Creating the table %s', self._table_name)

        self._table_instance.create()
        cf1 = self._table_instance.column_family(self._column_family_id)
        cf1.create()

    def Enqueue(self, messages, column_id):
        """Writes new rows to the given table.

        Args:
            messages: An array of strings that represents the message to be
                written to a new row in the table. Each message is writte to a
                new row
            column_id: A string that represents the name of the column to which
                data is to be written.
        """

        # Start writing rows
        logging.debug('Writing to the table : %s, column : %s', self._table_name,
                      column_id)
        for value in messages:
            row_key = str(self._end_index)
            self._end_index = self._end_index + 1
            row = self._table_instance.row(row_key)
            row.set_cell(self._column_family_id, column_id.encode('utf-8'),
                         value.encode('utf-8'))
            row.commit()
        # End writing rows

    def Dequeue(self):
        """Removes and returns the first row from the table.

        Returns:
            row: A row object that represents the top most row.
        """

        if self._end_index < self._start_index:
            return

        logging.info('Getting a single row by row key.')
        key = str(self._start_index)
        row_cond = self._table_instance.row(key)
        top_row = row_cond
        row_cond.delete()
        self._start_index = self._start_index + 1

        return top_row

    def DeleteTable(self):
        """Performs delete operation for a given table."""

        # Delete the table
        logging.debug('Deleting the table : %s', self._table_name)
        self._table_instance.delete()
