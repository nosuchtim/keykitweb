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
        console.log('js_fill_rect:', x, y, w, h, 'fillStyle:', ctx.fillStyle);
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

    // Global MIDI access object
    js_request_midi_access__deps: ['$stringToUTF8'],
    js_request_midi_access: function () {
        // MIDI is already initialized in preRun (in keykit_shell.html)
        // The devices are already in window.midiInputs and window.midiOutputs
        // This function is just a no-op now
        console.log('js_request_midi_access called (MIDI already initialized in preRun)');
    },

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

            // Call back into C code with MIDI data
            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_midi_message', null,
                             ['number', 'number', 'number', 'number'],
                             [index, status, data1, data2]);
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
            // Store key code for polling
            var keyCode = e.keyCode || e.which;
            window.keykitKeyBuffer.push(keyCode);

            // Call C callback if defined
            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_key_event', null,
                             ['number', 'number'],
                             [1, keyCode]);
            }
        });

        document.addEventListener('keyup', function(e) {
            var keyCode = e.keyCode || e.which;

            if (typeof Module !== 'undefined' && Module.ccall) {
                Module.ccall('mdep_on_key_event', null,
                             ['number', 'number'],
                             [0, keyCode]);
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
    }
});
