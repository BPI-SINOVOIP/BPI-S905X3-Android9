"""\
RPC request handler Django.  Exposed RPC interface functions should be
defined in rpc_interface.py.
"""

__author__ = 'showard@google.com (Steve Howard)'

import inspect
import pydoc
import re
import traceback
import urllib

from autotest_lib.client.common_lib import error
from autotest_lib.frontend.afe import models, rpc_utils
from autotest_lib.frontend.afe import rpcserver_logging
from autotest_lib.frontend.afe.json_rpc import serviceHandler

LOGGING_REGEXPS = [r'.*add_.*',
                   r'delete_.*',
                   r'.*remove_.*',
                   r'modify_.*',
                   r'create.*',
                   r'set_.*']
FULL_REGEXP = '(' + '|'.join(LOGGING_REGEXPS) + ')'
COMPILED_REGEXP = re.compile(FULL_REGEXP)

SHARD_RPC_INTERFACE = 'shard_rpc_interface'
COMMON_RPC_INTERFACE = 'common_rpc_interface'

def should_log_message(name):
    """Detect whether to log message.

    @param name: the method name.
    """
    return COMPILED_REGEXP.match(name)


class RpcMethodHolder(object):
    'Dummy class to hold RPC interface methods as attributes.'


class RpcValidator(object):
    """Validate Rpcs handled by RpcHandler.

    This validator is introduced to filter RPC's callers. If a caller is not
    allowed to call a given RPC, it will be refused by the validator.
    """
    def __init__(self, rpc_interface_modules):
        self._shard_rpc_methods = []
        self._common_rpc_methods = []

        for module in rpc_interface_modules:
            if COMMON_RPC_INTERFACE in module.__name__:
                self._common_rpc_methods = self._grab_name_from(module)

            if SHARD_RPC_INTERFACE in module.__name__:
                self._shard_rpc_methods = self._grab_name_from(module)


    def _grab_name_from(self, module):
        """Grab function name from module and add them to rpc_methods.

        @param module: an actual module.
        """
        rpc_methods = []
        for name in dir(module):
            if name.startswith('_'):
                continue
            attribute = getattr(module, name)
            if not inspect.isfunction(attribute):
                continue
            rpc_methods.append(attribute.func_name)

        return rpc_methods


    def validate_rpc_only_called_by_master(self, meth_name, remote_ip):
        """Validate whether the method name can be called by remote_ip.

        This funcion checks whether the given method (meth_name) belongs to
        _shard_rpc_module.

        If True, it then checks whether the caller's IP (remote_ip) is autotest
        master. An RPCException will be raised if an RPC method from
        _shard_rpc_module is called by a caller that is not autotest master.

        @param meth_name: the RPC method name which is called.
        @param remote_ip: the caller's IP.
        """
        if meth_name in self._shard_rpc_methods:
            global_afe_ip = rpc_utils.get_ip(rpc_utils.GLOBAL_AFE_HOSTNAME)
            if remote_ip != global_afe_ip:
                raise error.RPCException(
                        'Shard RPC %r cannot be called by remote_ip %s. It '
                        'can only be called by global_afe: %s' % (
                                meth_name, remote_ip, global_afe_ip))


    def encode_validate_result(self, meth_id, err):
        """Encode the return results for validator.

        It is used for encoding return response for RPC handler if caller of an
        RPC is refused by validator.

        @param meth_id: the id of the request for an RPC method.
        @param err: The error raised by validator.

        @return: a raw http response including the encoded error result. It
            will be parsed by service proxy.
        """
        error_result = serviceHandler.ServiceHandler.blank_result_dict()
        error_result['id'] = meth_id
        error_result['err'] = err
        error_result['err_traceback'] = traceback.format_exc()
        result = self.encode_result(error_result)
        return rpc_utils.raw_http_response(result)


class RpcHandler(object):
    """The class to handle Rpc requests."""

    def __init__(self, rpc_interface_modules, document_module=None):
        """Initialize an RpcHandler instance.

        @param rpc_interface_modules: the included rpc interface modules.
        @param document_module: the module includes documentation.
        """
        self._rpc_methods = RpcMethodHolder()
        self._dispatcher = serviceHandler.ServiceHandler(self._rpc_methods)
        self._rpc_validator = RpcValidator(rpc_interface_modules)

        # store all methods from interface modules
        for module in rpc_interface_modules:
            self._grab_methods_from(module)

        # get documentation for rpc_interface we can send back to the
        # user
        if document_module is None:
            document_module = rpc_interface_modules[0]
        self.html_doc = pydoc.html.document(document_module)


    def get_rpc_documentation(self):
        """Get raw response from an http documentation."""
        return rpc_utils.raw_http_response(self.html_doc)


    def raw_request_data(self, request):
        """Return raw data in request.

        @param request: the request to get raw data from.
        """
        if request.method == 'POST':
            return request.body
        return urllib.unquote(request.META['QUERY_STRING'])


    def execute_request(self, json_request):
        """Execute a json request.

        @param json_request: the json request to be executed.
        """
        return self._dispatcher.handleRequest(json_request)


    def decode_request(self, json_request):
        """Decode the json request.

        @param json_request: the json request to be decoded.
        """
        return self._dispatcher.translateRequest(json_request)


    def dispatch_request(self, decoded_request):
        """Invoke a RPC call from a decoded request.

        @param decoded_request: the json request to be processed and run.
        """
        return self._dispatcher.dispatchRequest(decoded_request)


    def log_request(self, user, decoded_request, decoded_result,
                    remote_ip, log_all=False):
        """Log request if required.

        @param user: current user.
        @param decoded_request: the decoded request.
        @param decoded_result: the decoded result.
        @param remote_ip: the caller's ip.
        @param log_all: whether to log all messages.
        """
        if log_all or should_log_message(decoded_request['method']):
            msg = '%s| %s:%s %s'  % (remote_ip, decoded_request['method'],
                                     user, decoded_request['params'])
            if decoded_result['err']:
                msg += '\n' + decoded_result['err_traceback']
                rpcserver_logging.rpc_logger.error(msg)
            else:
                rpcserver_logging.rpc_logger.info(msg)


    def encode_result(self, results):
        """Encode the result to translated json result.

        @param results: the results to be encoded.
        """
        return self._dispatcher.translateResult(results)


    def handle_rpc_request(self, request):
        """Handle common rpc request and return raw response.

        @param request: the rpc request to be processed.
        """
        remote_ip = self._get_remote_ip(request)
        user = models.User.current_user()
        json_request = self.raw_request_data(request)
        decoded_request = self.decode_request(json_request)

        # Validate whether method can be called by the remote_ip
        try:
            meth_id = decoded_request['id']
            meth_name = decoded_request['method']
            self._rpc_validator.validate_rpc_only_called_by_master(
                    meth_name, remote_ip)
        except KeyError:
            raise serviceHandler.BadServiceRequest(decoded_request)
        except error.RPCException as e:
            return self._rpc_validator.encode_validate_result(meth_id, e)

        decoded_request['remote_ip'] = remote_ip
        decoded_result = self.dispatch_request(decoded_request)
        result = self.encode_result(decoded_result)
        if rpcserver_logging.LOGGING_ENABLED:
            self.log_request(user, decoded_request, decoded_result,
                             remote_ip)
        return rpc_utils.raw_http_response(result)


    def handle_jsonp_rpc_request(self, request):
        """Handle the json rpc request and return raw response.

        @param request: the rpc request to be handled.
        """
        request_data = request.GET['request']
        callback_name = request.GET['callback']
        # callback_name must be a simple identifier
        assert re.search(r'^\w+$', callback_name)

        result = self.execute_request(request_data)
        padded_result = '%s(%s)' % (callback_name, result)
        return rpc_utils.raw_http_response(padded_result,
                                           content_type='text/javascript')


    @staticmethod
    def _allow_keyword_args(f):
        """\
        Decorator to allow a function to take keyword args even though
        the RPC layer doesn't support that.  The decorated function
        assumes its last argument is a dictionary of keyword args and
        passes them to the original function as keyword args.
        """
        def new_fn(*args):
            """Make the last argument as the keyword args."""
            assert args
            keyword_args = args[-1]
            args = args[:-1]
            return f(*args, **keyword_args)
        new_fn.func_name = f.func_name
        return new_fn


    def _grab_methods_from(self, module):
        for name in dir(module):
            if name.startswith('_'):
                continue
            attribute = getattr(module, name)
            if not inspect.isfunction(attribute):
                continue
            decorated_function = RpcHandler._allow_keyword_args(attribute)
            setattr(self._rpc_methods, name, decorated_function)


    def _get_remote_ip(self, request):
        """Get the ip address of a RPC caller.

        Returns the IP of the request, accounting for the possibility of
        being behind a proxy.
        If a Django server is behind a proxy, request.META["REMOTE_ADDR"] will
        return the proxy server's IP, not the client's IP.
        The proxy server would provide the client's IP in the
        HTTP_X_FORWARDED_FOR header.

        @param request: django.core.handlers.wsgi.WSGIRequest object.

        @return: IP address of remote host as a string.
                 Empty string if the IP cannot be found.
        """
        remote = request.META.get('HTTP_X_FORWARDED_FOR', None)
        if remote:
            # X_FORWARDED_FOR returns client1, proxy1, proxy2,...
            remote = remote.split(',')[0].strip()
        else:
            remote = request.META.get('REMOTE_ADDR', '')
        return remote
