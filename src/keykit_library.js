// JavaScript Library for KeyKit WebAssembly Port
// This file defines JavaScript functions that C code can call
// Based on patterns from examples/canvas_library.js
// Use with: emcc --js-library keykit_library.js

mergeInto(LibraryManager.library, {
    // ========== Canvas Drawing Functions ==========

    // Clear canvas
    js_clear_canvas: function () {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) {
            console.error('KeyKit canvas element not found!');
            return;
        }
        var ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, canvas.width, canvas.height);
    },

    // Draw line
    js_draw_line: function (x0, y0, x1, y1) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.moveTo(x0, y0);
        ctx.lineTo(x1, y1);
        ctx.stroke();
    },

    // Draw rectangle outline
    js_draw_rect: function (x, y, w, h) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.strokeRect(x, y, w, h);
    },

    // Draw filled rectangle
    js_fill_rect: function (x, y, w, h) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        // console.log('js_fill_rect:', x, y, w, h, 'fillStyle:', ctx.fillStyle);
        ctx.fillRect(x, y, w, h);
    },

    // Draw circle outline
    js_draw_circle: function (x, y, radius) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.arc(x, y, radius, 0, 2 * Math.PI);
        ctx.stroke();
    },

    // Draw filled circle
    js_fill_circle: function (x, y, radius) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.arc(x, y, radius, 0, 2 * Math.PI);
        ctx.fill();
    },

    // Draw ellipse outline
    js_draw_ellipse: function (x, y, radiusX, radiusY) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.ellipse(x, y, radiusX, radiusY, 0, 0, 2 * Math.PI);
        ctx.stroke();
    },

    // Fill ellipse
    js_fill_ellipse: function (x, y, radiusX, radiusY) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.beginPath();
        ctx.ellipse(x, y, radiusX, radiusY, 0, 0, 2 * Math.PI);
        ctx.fill();
    },

    // Draw text
    js_draw_text: function (x, y, text) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.fillText(UTF8ToString(text), x, y);
    },

    // Get font height (font metrics)
    js_get_font_height: function () {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) {
            console.log('js_get_font_height: canvas not found, returning default 16');
            return 16;  // default fallback
        }
        var ctx = canvas.getContext('2d');
        var metrics = ctx.measureText('M');

        console.log('js_get_font_height: current font = ' + ctx.font);
        console.log('js_get_font_height: metrics =', metrics);

        // Use actualBoundingBoxAscent + actualBoundingBoxDescent if available
        // Otherwise estimate from font size
        if (metrics.actualBoundingBoxAscent !== undefined &&
            metrics.actualBoundingBoxDescent !== undefined) {
            var height = Math.ceil(metrics.actualBoundingBoxAscent + metrics.actualBoundingBoxDescent);
            console.log('js_get_font_height: using bounding box: ascent=' + metrics.actualBoundingBoxAscent +
                       ' descent=' + metrics.actualBoundingBoxDescent + ' total=' + height);
            return height;
        }
        // Fallback: extract font size from font string (e.g., "16px monospace")
        var match = ctx.font.match(/(\d+)px/);
        var fontSize = match ? parseInt(match[1]) : 16;
        console.log('js_get_font_height: using font size from string: ' + fontSize);
        return fontSize;
    },

    // Get font width (measure width of 'M' character)
    js_get_font_width: function () {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) {
            console.log('js_get_font_width: canvas not found, returning default 8');
            return 8;  // default fallback
        }
        var ctx = canvas.getContext('2d');
        var metrics = ctx.measureText('M');
        var width = Math.ceil(metrics.width);
        // console.log('js_get_font_width: current font = ' + ctx.font + ', width = ' + width);
        return width;
    },

    // Draw filled polygon
    js_fill_polygon: function (xPoints, yPoints, numPoints) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');

        if (numPoints < 3) return;

        ctx.beginPath();
        var x = getValue(xPoints, 'i32');
        var y = getValue(yPoints, 'i32');
        ctx.moveTo(x, y);

        for (var i = 1; i < numPoints; i++) {
            x = getValue(xPoints + i * 4, 'i32');
            y = getValue(yPoints + i * 4, 'i32');
            ctx.lineTo(x, y);
        }

        ctx.closePath();
        ctx.fill();
    },

    // Set drawing color (stroke and fill)
    js_set_color: function (color) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        var colorStr = UTF8ToString(color);
        ctx.strokeStyle = colorStr;
        ctx.fillStyle = colorStr;
    },

    // Set stroke color only
    js_set_stroke_color: function (color) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.strokeStyle = UTF8ToString(color);
    },

    // Set fill color only
    js_set_fill_color: function (color) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.fillStyle = UTF8ToString(color);
    },

    // Set line width
    js_set_line_width: function (width) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.lineWidth = width;
    },

    // Set font
    js_set_font: function (font) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.font = UTF8ToString(font);
    },

    // Get canvas width
    js_get_canvas_width: function () {
        var canvas = document.getElementById('keykit-canvas');
        return canvas ? canvas.width : 0;
    },

    // Get canvas height
    js_get_canvas_height: function () {
        var canvas = document.getElementById('keykit-canvas');
        return canvas ? canvas.height : 0;
    },

    // Set global alpha (transparency)
    js_set_alpha: function (alpha) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.globalAlpha = alpha;
    },

    // Save context state
    js_save_context: function () {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.save();
    },

    // Restore context state
    js_restore_context: function () {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.restore();
    },

    // Set composite operation (for XOR mode, etc.)
    js_set_composite_operation: function (operation) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        ctx.globalCompositeOperation = UTF8ToString(operation);
    },

    // ========== Web MIDI API Functions ==========

    // Get number of MIDI input devices
    js_get_midi_input_count: function () {
        if (!window.midiInputs) {
            return 0;
        }
        return window.midiInputs.length;
    },

    // Get number of MIDI output devices
    js_get_midi_output_count: function () {
        if (!window.midiOutputs) {
            return 0;
        }
        return window.midiOutputs.length;
    },

    // Get MIDI input device name
    js_get_midi_input_name__deps: ['$stringToUTF8'],
    js_get_midi_input_name: function (index, buffer, buffer_size) {
        if (!window.midiInputs || index < 0 || index >= window.midiInputs.length) {
            stringToUTF8('Unknown', buffer, buffer_size);
            return;
        }
        var device = window.midiInputs[index];
        var name = device.name || 'MIDI Input ' + index;
        stringToUTF8(name, buffer, buffer_size);
    },

    // Get MIDI output device name
    js_get_midi_output_name__deps: ['$stringToUTF8'],
    js_get_midi_output_name: function (index, buffer, buffer_size) {
        if (!window.midiOutputs || index < 0 || index >= window.midiOutputs.length) {
            stringToUTF8('Unknown', buffer, buffer_size);
            return;
        }
        var device = window.midiOutputs[index];
        var name = device.name || 'MIDI Output ' + index;
        stringToUTF8(name, buffer, buffer_size);
    },

    // Open a MIDI input device and set up message listener
    js_open_midi_input: function (index) {
        if (!window.midiInputs || index < 0 || index >= window.midiInputs.length) {
            console.error('Invalid MIDI input index: ' + index);
            return -1;
        }

        var input = window.midiInputs[index];
        console.log('Opening MIDI input: ' + input.name);

        // Set up message handler
        input.onmidimessage = function(event) {
            var data = event.data;
            var status = data[0];
            var data1 = data.length > 1 ? data[1] : 0;
            var data2 = data.length > 2 ? data[2] : 0;

            // Log MIDI input for debugging
            var msgType = '';
            if ((status & 0xF0) === 0x90 && data2 > 0) msgType = 'Note On';
            else if ((status & 0xF0) === 0x80 || ((status & 0xF0) === 0x90 && data2 === 0)) msgType = 'Note Off';
            else if ((status & 0xF0) === 0xB0) msgType = 'Control Change';
            else if ((status & 0xF0) === 0xE0) msgType = 'Pitch Bend';
            else if ((status & 0xF0) === 0xD0) msgType = 'Channel Pressure';
            else if ((status & 0xF0) === 0xA0) msgType = 'Poly Aftertouch';
            else if ((status & 0xF0) === 0xC0) msgType = 'Program Change';
            else msgType = 'Other';

            // console.log('[MIDI IN] Device ' + index + ' (' + input.name + '): ' + msgType +
            //            ' - Status: 0x' + status.toString(16).padStart(2, '0') +
            //            ', Data1: ' + data1 + ', Data2: ' + data2);

            // Call back into C code with MIDI data
            if (typeof Module !== 'undefined' && Module.ccall) {
                try {
                    Module.ccall('mdep_on_midi_message', null,
                                 ['number', 'number', 'number', 'number'],
                                 [index, status, data1, data2]);
                    // console.log('[MIDI IN] Successfully called C callback');
                } catch (e) {
                    console.error('[MIDI IN] Error calling C callback:', e);
                }
            } else {
                console.warn('[MIDI IN] Module not ready, message dropped');
            }
        };

        return 0; // Success
    },

    // Close a MIDI input device
    js_close_midi_input: function (index) {
        if (!window.midiInputs || index < 0 || index >= window.midiInputs.length) {
            return -1;
        }

        var input = window.midiInputs[index];
        input.onmidimessage = null;
        console.log('Closed MIDI input: ' + input.name);
        return 0;
    },

    // Send MIDI message to output device
    js_send_midi_output: function (index, data_ptr, data_len) {
        if (!window.midiOutputs || index < 0 || index >= window.midiOutputs.length) {
            console.error('Invalid MIDI output index: ' + index);
            return -1;
        }

        var output = window.midiOutputs[index];

        // Convert C byte array to JavaScript array
        var data = [];
        for (var i = 0; i < data_len; i++) {
            data.push(HEAPU8[data_ptr + i]);
        }

        try {
            output.send(data);
            return 0; // Success
        } catch (err) {
            console.error('Error sending MIDI data:', err);
            return -1;
        }
    },

    // ========== Mouse Event Functions ==========

    // Global mouse state
    js_setup_mouse_events: function () {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) {
            console.error('KeyKit canvas element not found!');
            return;
        }

        window.keykitMouseX = 0;
        window.keykitMouseY = 0;
        window.keykitMouseButtons = 0;

        // Mouse move event
        canvas.addEventListener('mousemove', function(e) {
            var rect = canvas.getBoundingClientRect();
            window.keykitMouseX = Math.floor(e.clientX - rect.left);
            window.keykitMouseY = Math.floor(e.clientY - rect.top);

            // Call C callback if defined
            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_mouse_move', null,
                             ['number', 'number'],
                             [window.keykitMouseX, window.keykitMouseY]);
            }
        });

        // Mouse button events
        canvas.addEventListener('mousedown', function(e) {
            var rect = canvas.getBoundingClientRect();
            window.keykitMouseX = Math.floor(e.clientX - rect.left);
            window.keykitMouseY = Math.floor(e.clientY - rect.top);

            // Set button bit (0=left, 1=middle, 2=right)
            window.keykitMouseButtons |= (1 << e.button);

            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_mouse_button', null,
                             ['number', 'number', 'number', 'number'],
                             [1, window.keykitMouseX, window.keykitMouseY, window.keykitMouseButtons]);
            }
        });

        canvas.addEventListener('mouseup', function(e) {
            var rect = canvas.getBoundingClientRect();
            window.keykitMouseX = Math.floor(e.clientX - rect.left);
            window.keykitMouseY = Math.floor(e.clientY - rect.top);

            // Clear button bit
            window.keykitMouseButtons &= ~(1 << e.button);

            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_mouse_button', null,
                             ['number', 'number', 'number', 'number'],
                             [0, window.keykitMouseX, window.keykitMouseY, window.keykitMouseButtons]);
            }
        });

        // Prevent context menu
        canvas.addEventListener('contextmenu', function(e) {
            e.preventDefault();
        });

        console.log('Mouse event listeners set up successfully');
    },

    // Get current mouse position and buttons
    js_get_mouse_state: function (x_ptr, y_ptr, buttons_ptr) {
        setValue(x_ptr, window.keykitMouseX || 0, 'i32');
        setValue(y_ptr, window.keykitMouseY || 0, 'i32');
        setValue(buttons_ptr, window.keykitMouseButtons || 0, 'i32');
        return 0;
    },

    // ========== Keyboard Event Functions ==========

    js_setup_keyboard_events: function () {
        window.keykitKeyBuffer = [];

        document.addEventListener('keydown', function(e) {
            // Ignore modifier keys (Shift, Ctrl, Alt, Meta, etc.)
            if (e.key === 'Shift' || e.key === 'Control' || e.key === 'Alt' ||
                e.key === 'Meta' || e.key === 'CapsLock' || e.key === 'Tab' ||
                e.key === 'NumLock' || e.key === 'ScrollLock') {
                return;
            }

            // Prevent browser from handling keyboard shortcuts
            e.preventDefault();

            // Use e.key to get the actual character (respects shift, caps lock, etc.)
            // For single characters, use charCodeAt to get the ASCII/Unicode value
            var keyCode;
            if (e.key && e.key.length === 1) {
                // Single character - get its character code (respects case)
                keyCode = e.key.charCodeAt(0);
            } else {
                // Special keys - use keyCode
                keyCode = e.keyCode || e.which;
            }

            window.keykitKeyBuffer.push(keyCode);

            // Call C callback with modifier key state
            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_key_event', null,
                             ['number', 'number', 'number', 'number', 'number'],
                             [1, keyCode, e.ctrlKey ? 1 : 0, e.shiftKey ? 1 : 0, e.altKey ? 1 : 0]);
            }
        });

        document.addEventListener('keyup', function(e) {
            // Ignore modifier keys
            if (e.key === 'Shift' || e.key === 'Control' || e.key === 'Alt' ||
                e.key === 'Meta' || e.key === 'CapsLock' || e.key === 'Tab' ||
                e.key === 'NumLock' || e.key === 'ScrollLock') {
                return;
            }

            // Prevent browser from handling keyboard shortcuts
            e.preventDefault();

            var keyCode;
            if (e.key && e.key.length === 1) {
                keyCode = e.key.charCodeAt(0);
            } else {
                keyCode = e.keyCode || e.which;
            }

            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_key_event', null,
                             ['number', 'number', 'number', 'number', 'number'],
                             [0, keyCode, e.ctrlKey ? 1 : 0, e.shiftKey ? 1 : 0, e.altKey ? 1 : 0]);
            }
        });

        console.log('Keyboard event listeners set up successfully');
    },

    // Get next key from buffer
    js_get_key: function () {
        if (!window.keykitKeyBuffer || window.keykitKeyBuffer.length === 0) {
            return -1;
        }
        return window.keykitKeyBuffer.shift();
    },

    // Check if key is available
    js_has_key: function () {
        return (window.keykitKeyBuffer && window.keykitKeyBuffer.length > 0) ? 1 : 0;
    },

    // ========== Bitmap Functions ==========

    // Get image data from canvas and copy to C buffer
    js_get_image_data: function (x, y, width, height, buffer) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return 0;
        var ctx = canvas.getContext('2d');

        try {
            var imageData = ctx.getImageData(x, y, width, height);
            // Copy RGBA pixel data to C buffer
            Module.HEAPU8.set(imageData.data, buffer);
            return imageData.data.length; // Return number of bytes
        } catch (e) {
            console.error('js_get_image_data error:', e);
            return 0;
        }
    },

    // Put image data from C buffer to canvas
    js_put_image_data: function (buffer, bufLen, x, y, width, height) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');

        try {
            var imageData = ctx.createImageData(width, height);
            // Copy from C buffer to ImageData
            var srcData = new Uint8Array(Module.HEAPU8.buffer, buffer, bufLen);
            imageData.data.set(srcData);
            ctx.putImageData(imageData, x, y);
        } catch (e) {
            console.error('js_put_image_data error:', e);
        }
    },

    // Copy bitmap region (for mdep_movebitmap)
    js_copy_bitmap_region: function (fromx, fromy, width, height, tox, toy) {
        var canvas = document.getElementById('keykit-canvas');
        if (!canvas) return;
        var ctx = canvas.getContext('2d');

        try {
            // Get the source region
            var imageData = ctx.getImageData(fromx, fromy, width, height);
            // Put it at the destination
            ctx.putImageData(imageData, tox, toy);
        } catch (e) {
            console.error('js_copy_bitmap_region error:', e);
        }
    },

    // ========== NATS Messaging Functions ==========

    // Connect to NATS server
    js_nats_connect__deps: ['$UTF8ToString'],
    js_nats_connect: function (urlPtr) {
        var url = UTF8ToString(urlPtr);
        console.log('[NATS] Attempting to connect to ' + url);

        // Check if NATS library is available
        if (typeof nats === 'undefined') {
            console.error('[NATS] NATS.ws library not loaded! Include nats.js in HTML.');
            return -1;
        }

        // Initialize StringCodec for encoding/decoding messages
        if (!window.natsStringCodec) {
            window.natsStringCodec = nats.StringCodec();
        }
        var sc = window.natsStringCodec;

        // Connect to NATS server
        nats.connect({ servers: url })
            .then(function(nc) {
                window.natsConnection = nc;
                console.log('[NATS] Connected to ' + url);

                // Set up default subscription to 'keykit.>' (all keykit subjects)
                var sub = nc.subscribe('keykit.>', {
                    callback: function(err, msg) {
                        if (err) {
                            console.error('[NATS] Subscription error:', err);
                            return;
                        }
                        var data = sc.decode(msg.data);
                        console.log('[NATS] Received message on "' + msg.subject + '": ' + data);

                        // Call C callback if available
                        if (typeof Module !== 'undefined' && Module.ccall) {
                            try {
                                Module.ccall('mdep_on_nats_message', null,
                                             ['string', 'string'],
                                             [msg.subject, data]);
                                console.log('[NATS] Successfully called C callback');
                            } catch (e) {
                                console.error('[NATS] Error calling C callback:', e);
                            }
                        } else {
                            console.warn('[NATS] Module not ready, message dropped');
                        }
                    }
                });

                console.log('[NATS] Subscribed to keykit.* subjects');

                // Handle connection closed
                nc.closed().then(function(err) {
                    if (err) {
                        console.error('[NATS] Connection closed with error:', err);
                    } else {
                        console.log('[NATS] Connection closed gracefully');
                    }
                    window.natsConnection = null;
                });
            })
            .catch(function(err) {
                console.error('[NATS] Connection failed:', err);
                window.natsConnection = null;
            });

        return 0; // Initiated (async)
    },

    // Publish message to NATS subject
    js_nats_publish__deps: ['$UTF8ToString'],
    js_nats_publish: function (subjectPtr, dataPtr) {
        if (!window.natsConnection) {
            console.error('[NATS] Not connected - cannot publish');
            return -1;
        }

        var subject = UTF8ToString(subjectPtr);
        var data = UTF8ToString(dataPtr);
        var sc = window.natsStringCodec;

        try {
            window.natsConnection.publish(subject, sc.encode(data));
            console.log('[NATS] Published to "' + subject + '": ' + data);
            return 0; // Success
        } catch (e) {
            console.error('[NATS] Publish error:', e);
            return -1;
        }
    },

    // Subscribe to specific NATS subject
    js_nats_subscribe__deps: ['$UTF8ToString'],
    js_nats_subscribe: function (subjectPtr) {
        if (!window.natsConnection) {
            console.error('[NATS] Not connected - cannot subscribe');
            return -1;
        }

        var subject = UTF8ToString(subjectPtr);
        var sc = window.natsStringCodec;

        try {
            var sub = window.natsConnection.subscribe(subject, {
                callback: function(err, msg) {
                    if (err) {
                        console.error('[NATS] Subscription error on "' + subject + '":', err);
                        return;
                    }
                    var data = sc.decode(msg.data);
                    console.log('[NATS] Received on "' + msg.subject + '": ' + data);

                    // Call C callback
                    if (typeof Module !== 'undefined' && Module.ccall) {
                        try {
                            Module.ccall('mdep_on_nats_message', null,
                                         ['string', 'string'],
                                         [msg.subject, data]);
                        } catch (e) {
                            console.error('[NATS] Error calling C callback:', e);
                        }
                    }
                }
            });

            console.log('[NATS] Subscribed to "' + subject + '"');
            return 0; // Success
        } catch (e) {
            console.error('[NATS] Subscribe error:', e);
            return -1;
        }
    },

    // Check if connected to NATS
    js_nats_is_connected: function () {
        return (window.natsConnection && !window.natsConnection.isClosed()) ? 1 : 0;
    },

    // Close NATS connection
    js_nats_close: function () {
        if (!window.natsConnection) {
            console.log('[NATS] No active connection to close');
            return 0;
        }

        try {
            window.natsConnection.close();
            console.log('[NATS] Connection closed');
            window.natsConnection = null;
            return 0;
        } catch (e) {
            console.error('[NATS] Error closing connection:', e);
            return -1;
        }
    },

    // ========== WebSocket Functions (for TCP/IP and UDP simulation) ==========

    // Create WebSocket connection (simulates tcpip_connect)
    js_websocket_connect__deps: ['$UTF8ToString'],
    js_websocket_connect: function (urlPtr, portId) {
        var url = UTF8ToString(urlPtr);
        console.log('[WebSocket] Connecting to ' + url + ' for port ' + portId);

        try {
            var ws = new WebSocket(url);

            // Store in global registry
            if (!window.keykitWebSockets) {
                window.keykitWebSockets = {};
            }
            window.keykitWebSockets[portId] = {
                ws: ws,
                state: 'connecting',
                receiveBuffer: []
            };

            ws.onopen = function() {
                console.log('[WebSocket] Connected: ' + url);
                window.keykitWebSockets[portId].state = 'connected';

                // Notify C code
                if (typeof Module !== 'undefined' && Module.ccall) {
                    try {
                        Module.ccall('mdep_on_websocket_event', null,
                                     ['number', 'string'],
                                     [portId, 'open']);
                    } catch (e) {
                        console.error('[WebSocket] Error calling C callback:', e);
                    }
                }
            };

            ws.onmessage = function(event) {
                console.log('[WebSocket] Received data on port ' + portId + ': ' + event.data.length + ' bytes');

                // Buffer the data
                window.keykitWebSockets[portId].receiveBuffer.push(event.data);

                // Notify C code
                if (typeof Module !== 'undefined' && Module.ccall) {
                    try {
                        Module.ccall('mdep_on_websocket_event', null,
                                     ['number', 'string'],
                                     [portId, 'data']);
                    } catch (e) {
                        console.error('[WebSocket] Error calling C callback:', e);
                    }
                }
            };

            ws.onerror = function(error) {
                console.error('[WebSocket] Error on port ' + portId + ':', error);
                window.keykitWebSockets[portId].state = 'error';
            };

            ws.onclose = function() {
                console.log('[WebSocket] Closed: port ' + portId);
                window.keykitWebSockets[portId].state = 'closed';

                // Notify C code
                if (typeof Module !== 'undefined' && Module.ccall) {
                    try {
                        Module.ccall('mdep_on_websocket_event', null,
                                     ['number', 'string'],
                                     [portId, 'close']);
                    } catch (e) {
                        console.error('[WebSocket] Error calling C callback:', e);
                    }
                }
            };

            return 0; // Success (async)
        } catch (e) {
            console.error('[WebSocket] Connection failed:', e);
            return -1;
        }
    },

    // Send data via WebSocket
    js_websocket_send__deps: ['$UTF8ToString'],
    js_websocket_send: function (portId, dataPtr, dataLen) {
        if (!window.keykitWebSockets || !window.keykitWebSockets[portId]) {
            console.error('[WebSocket] Port ' + portId + ' not found');
            return -1;
        }

        var wsInfo = window.keykitWebSockets[portId];
        if (wsInfo.state !== 'connected') {
            console.error('[WebSocket] Port ' + portId + ' not connected (state: ' + wsInfo.state + ')');
            return -1;
        }

        try {
            // Convert C buffer to JavaScript string or binary
            var data = new Uint8Array(Module.HEAPU8.buffer, dataPtr, dataLen);
            wsInfo.ws.send(data);
            console.log('[WebSocket] Sent ' + dataLen + ' bytes on port ' + portId);
            return dataLen;
        } catch (e) {
            console.error('[WebSocket] Send error on port ' + portId + ':', e);
            return -1;
        }
    },

    // Receive data from WebSocket buffer
    js_websocket_receive__deps: ['$stringToUTF8'],
    js_websocket_receive: function (portId, bufferPtr, bufferSize) {
        if (!window.keykitWebSockets || !window.keykitWebSockets[portId]) {
            return 0; // No data
        }

        var wsInfo = window.keykitWebSockets[portId];
        if (wsInfo.receiveBuffer.length === 0) {
            return 0; // No data
        }

        // Get first buffered message
        var data = wsInfo.receiveBuffer.shift();

        // Convert to bytes and copy to C buffer
        var bytes;
        if (typeof data === 'string') {
            // Text data
            var encoder = new TextEncoder();
            bytes = encoder.encode(data);
        } else if (data instanceof ArrayBuffer) {
            bytes = new Uint8Array(data);
        } else if (data instanceof Blob) {
            // For Blob, we'd need async handling - for now, skip
            console.warn('[WebSocket] Blob data not yet supported');
            return 0;
        } else {
            bytes = new Uint8Array(data);
        }

        var bytesToCopy = Math.min(bytes.length, bufferSize);
        Module.HEAPU8.set(bytes.subarray(0, bytesToCopy), bufferPtr);

        console.log('[WebSocket] Received ' + bytesToCopy + ' bytes from port ' + portId);
        return bytesToCopy;
    },

    // Check WebSocket state
    js_websocket_state: function (portId) {
        if (!window.keykitWebSockets || !window.keykitWebSockets[portId]) {
            return 0; // Not found
        }

        var wsInfo = window.keykitWebSockets[portId];
        switch (wsInfo.state) {
            case 'connecting': return 1;
            case 'connected': return 2;
            case 'closing': return 3;
            case 'closed': return 4;
            case 'error': return 5;
            default: return 0;
        }
    },

    // Close WebSocket
    js_websocket_close: function (portId) {
        if (!window.keykitWebSockets || !window.keykitWebSockets[portId]) {
            return 0;
        }

        try {
            var wsInfo = window.keykitWebSockets[portId];
            if (wsInfo.ws && wsInfo.ws.readyState !== WebSocket.CLOSED) {
                wsInfo.ws.close();
            }
            delete window.keykitWebSockets[portId];
            console.log('[WebSocket] Closed port ' + portId);
            return 0;
        } catch (e) {
            console.error('[WebSocket] Error closing port ' + portId + ':', e);
            return -1;
        }
    }
});
