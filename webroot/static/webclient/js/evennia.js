/*
Evenna webclient library

This javascript library handles all communication between Evennia and
whatever client front end is used.

The library will try to communicate with Evennia using websockets
(evennia/server/portal/webclient.py). However, if the web browser is
old and does not support websockets, it will instead fall back to a
long-polling (AJAX/COMET) type of connection (using
evennia/server/portal/webclient_ajax.py)

All messages are valid JSON arrays on this single form:

    ["cmdname", args, kwargs],

where args is an JSON array and kwargs is a JSON object. These will be both
used as arguments emitted to a callback named "cmdname" as cmdname(args, kwargs).

This library makes the "Evennia" object available. It has the
following official functions:

   - Evennia.init(options)
        This stores the connections/emitters and creates the websocket/ajax connection.
        This can be called as often as desired - the lib will still only be
        initialized once. The argument is an js object with the following possible keys:
            'connection': This defaults to Evennia.WebsocketConnection but
                can also be set to Evennia.CometConnection for backwards
                compatibility. See below.
            'emitter': An optional custom command handler for distributing
                data from the server to suitable listeners. If not given,
                a default will be used.
   - Evennia.msg(funcname, [args,...], callback)
        Send a command to the server. You can also provide a function
        to call with the return of the call (note that commands will
        not return anything unless specified to do so server-side).

A "connection" object must have the method
    - msg(data) - this should relay data to the Server. This function should itself handle
        the conversion to JSON before sending across the wire.
    - When receiving data from the Server (always data = [cmdname, args, kwargs]), this must be
        JSON-unpacked and the result redirected to Evennia.emit(data[0], data[1], data[2]).
An "emitter" object must have a function
    - emit(cmdname, args, kwargs) - this will be called by the backend and is expected to
        relay the data to its correct gui element.
    - The default emitter also has the following methods:
        - on(cmdname, listener) - this ties a listener to the backend. This function
            should be called as listener(args, kwargs) when the backend calls emit.
        - off(cmdname) - remove the listener for this cmdname.
*/

import io from 'https://cdn.socket.io/4.7.4/socket.io.esm.min.js';

(function() {
    "use strict"
    var cmdid = 0;
    var cmdmap = {};

    var Evennia = {

        debug: true,
        initialized: false,

        // Initialize the Evennia object with emitter and connection.
        //
        // Args:
        //   opts (obj):
        //       emitter - custom emitter. If not given,
        //          will use a default emitter. Must have
        //          an "emit" function.
        //       connection - This defaults to using either
        //          a WebsocketConnection or a AjaxCometConnection
        //          depending on what the browser supports. If given
        //          it must have a 'msg' method and make use of
        //          Evennia.emit to return data to Client.
        //
        init: function(opts) {
            if (this.initialized) {
                // make it safe to call multiple times.
                return;
            }
            this.initialized = true;

            opts = opts || {};
            this.emitter = opts.emitter || new DefaultEmitter();

            if (opts.connection) {
               this.connection = opts.connection;
            }
            else if (window.WebSocket && wsactive) {
                this.connection = new WebsocketConnection();
            } else {
                // ORIGINAL: this.connection = new AjaxCometConnection();
            }
            log('Evennia initialized.')
        },

        // Connect to the Evennia server.
        // Re-establishes the connection after it is lost.
        //
        connect: function() {
            if (this.connection.isOpen()) {
                // Already connected.
                return;
            }
            this.connection.connect();
            log('Evennia reconnecting.')
        },

        // Returns true if the connection is open.
        //
        isConnected: function () {
          return this.connection.isOpen();
        },

        // client -> Evennia.
        // called by the frontend to send a command to Evennia.
        //
        // args:
        //   cmdname (str): String identifier to call
        //   kwargs (obj): Data argument for calling as cmdname(kwargs)
        //   callback (func): If given, will be given an eventual return
        //      value from the backend.
        //
        msg: function (cmdname, args, kwargs, callback) {
            if (!cmdname) {
                return;
            }
            if (kwargs) {
                kwargs.cmdid = cmdid++;
            }
            var outargs = args ? args : [];
            var outkwargs = kwargs ? kwargs : {};
            var data = [cmdname, outargs, outkwargs];

            if (typeof callback === 'function') {
                cmdmap[cmdid] = callback;
            }

            if(cmdname === "text") {
                cmdname = "Game.Command";
                outkwargs = {data: args[0]}
            } else {
                outkwargs["args"] = outargs;
            }

            this.connection.msg(cmdname, outkwargs);

        },

        // Evennia -> Client.
        // Called by the backend to send the data to the
        // emitter, which in turn distributes it to its
        // listener(s).
        //
        // Args:
        //   event (event): Event received from Evennia
        //   args (array): Arguments to listener
        //   kwargs (obj): keyword-args to listener
        //
        emit: function (cmdname, args, kwargs) {
            if (kwargs.cmdid && (kwargs.cmdid in cmdmap)) {
                cmdmap[kwargs.cmdid].apply(this, [args, kwargs]);
                delete cmdmap[kwargs.cmdid];
            }
            else {
                this.emitter.emit(cmdname, args, kwargs);
            }
        },

    }; // end of evennia object


    // Basic emitter to distribute data being sent to the client from
    // the Server. An alternative can be overridden by giving it
    // in Evennia.init({emitter:myemitter})
    //
    var DefaultEmitter = function () {
        var listeners = {};
        // Emit data to all listeners tied to a given cmdname.
        // If the cmdname is not recognized, call a listener
        // named 'default' with arguments [cmdname, args, kwargs].
        // If no 'default' is found, ignore silently.
        //
        // Args:
        //   cmdname (str): Name of command, used to find
        //     all listeners to this call; each will be
        //     called as function(kwargs).
        //   kwargs (obj): Argument to the listener.
        //
        var emit = function (cmdname, args, kwargs) {
            if (listeners[cmdname]) {
                listeners[cmdname].apply(this, [args, kwargs]);
            }
            else if (listeners["default"]) {
                listeners["default"].apply(this, [cmdname, args, kwargs]);
            }
        };

        // Bind listener to event
        //
        // Args:
        //   cmdname (str): Name of event to handle.
        //   listener (function): Function taking one argument,
        //     to listen to cmdname events.
        //
        var on = function (cmdname, listener) {
            if (typeof(listener) === 'function') {
                listeners[cmdname] = listener;
            };
        };

        // remove handling of this cmdname
        //
        // Args:
        //   cmdname (str): Name of event to handle
        //
        var off = function (cmdname) {
            delete listeners[cmdname]
        };
        return {emit:emit, on:on, off:off};
    };

    // Websocket Connector
    //
    var WebsocketConnection = function () {
        var open = false;
        var socket = null;

        const protocol = window.location.protocol.includes('https') ? 'https' : 'http';
        const host = window.location.hostname;
        const port = window.location.port || (protocol === 'https' ? 443 : 80);
        const socketUrl = `${protocol}://${host}:${port}`;

        var connect = function() {
            if (socket && socket.connected) {
                // No-op if a connection is already open.
                return;
            }
            // Important - we pass csessid tacked on the url
            //websocket = new WebSocket(wsurl + '?' + csessid + '&' + browser);
            socket = io(socketUrl);

            // Handle Websocket open event
            socket.on('connect', function() {
                Evennia.emit("connection_open", ["socketio"], {});
            });

            socket.on("disconnect", function() {
                Evennia.emit("connection_close", ["socketio"], {});
            });

            socket.on("Game.Text", function(message) {
                if(message?.data) {
                    Evennia.emit("text", [message.data], {});
                }
            });

            socket.on("Game.GMCP", function(message) {
               // Not yet implemented.
            });
        }

        var msg = function(event, data) {
            // send data across the wire. Make sure to json it.
            // console.log("client->server:", data)
            socket.emit(event, data);
        };

        var close = function() {
            // tell the server this connection is closing (usually
            // tied to when the client window is closed). This
            // Makes use of a websocket-protocol specific instruction.
            socket.close();
            socket = null;
            open = false;
        }

        var isOpen = function() {
            return (socket && socket.connected);
        }

        connect();

        return {connect: connect, msg: msg, close: close, isOpen: isOpen};
    };


    window.Evennia = Evennia;

})(); // end of auto-calling Evennia object defintion

// helper logging function (requires a js dev-console in the browser)
function log() {
  if (Evennia.debug) {
    console.log(JSON.stringify(arguments));
  }
}


// figure out the browser info string
var browser = (function (agent) {
    "use strict"
    switch (true) {
        case agent.indexOf("edge") > -1: return "edge";
        case agent.indexOf("edg") > -1: return "chromium based edge (dev or canary)";
        case agent.indexOf("opr") > -1 && !!window.opr: return "opera";
        case agent.indexOf("chrome") > -1 && !!window.chrome: return "chrome";
        case agent.indexOf("trident") > -1: return "ie";
        case agent.indexOf("firefox") > -1: return "firefox";
        case agent.indexOf("safari") > -1: return "safari";
        default: return "other";
    }
})(window.navigator.userAgent.toLowerCase());
console.log(window.navigator.userAgent.toLowerCase() + "\n" + browser);


// Called when page has finished loading (kicks the client into gear)
$(document).ready(function() {
    setTimeout( function () {
        // the short timeout supposedly causes the load indicator
        // in Chrome to stop spinning
        Evennia.init()
        },
        500
    );
});
