import logging
import socket

_BUF_SIZE = 1024

class FakePrinter():
    """A fake printer (server)."""
    sock = 0
    def __init__(self):
        """Initialize fake printer"""
        # Create a TCP/IP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Bind the socket to the port
        server_address = ('localhost', 9100)
        self.sock.bind(server_address)

    def start(self, log_file_path):
        """Start the fake printer.

        It listens on port 9100 and dump printing request
        received to a temporary file.

        Args:
            @param log_file_path: the abs path of log file.
        """
        # Listen for incoming printer request.
        self.sock.listen(1)
        logging.info('waiting for a printing request')
        (connection, client_address) = self.sock.accept()
        try:
            logging.info('printing request from ' + str(client_address))
            logfile = open(log_file_path, 'w');
            while True:
                data = connection.recv(_BUF_SIZE)
                if not data:
                    logging.info('no more data from ' + str(client_address))
                    break
                logfile.write(data)
            logfile.close();
            logging.info('printing request is dumped to ' + str(logfile))
        finally:
            connection.close()
