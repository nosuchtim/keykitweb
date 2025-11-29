#!/usr/bin/env python3
"""
Threaded HTTP server for serving KeyKit WebAssembly files.
Handles many concurrent connections better than the default http.server.

Usage: python serve.py [port]
Default port is 8000
"""

import sys
from http.server import HTTPServer, SimpleHTTPRequestHandler
from socketserver import ThreadingMixIn

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in separate threads."""
    daemon_threads = True

def run(port=8000):
    server_address = ('', port)
    httpd = ThreadedHTTPServer(server_address, SimpleHTTPRequestHandler)
    print(f"Serving on http://localhost:{port}")
    print("Press Ctrl+C to stop")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down...")
        httpd.shutdown()

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    run(port)
