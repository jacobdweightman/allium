import sys
from typing import Any
import unittest
import json
import subprocess

alliumLSP = sys.argv[1]

class LSPClient:
    def __init__(self, serverStdIn, serverStdOut):
        self.requestCount = 0
        self.serverStdIn = serverStdIn
        self.serverStdOut = serverStdOut
    
    def encode(self, method: str, value: Any) -> bytes:
        content = json.dumps({
            "jsonrpc": "2.0",
            "id": self.requestCount,
            "method": method,
            "params": value
        }, separators=(',', ':'))
        length = len(content)
        self.requestCount += 1
        return f"Content-Length: {length}\n\n{content}".encode('utf-8')
    
    def send(self, method: str, params: Any) -> str:
        query = self.encode(method, params)
        self.serverStdIn.write(query)
        self.serverStdIn.flush()
        print("query:", query)
        content_length = self.serverStdOut.readline() # Content-Length: ...\n
        print(f"Content Length: {content_length}")
        self.serverStdOut.readline() # \n
        response = self.serverStdOut.readline().decode('utf-8')
        print("output:", response)
        return response


class LanguageServerTest(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()
        self.serverProc = subprocess.Popen(
            [alliumLSP],
            stdin=subprocess.PIPE,
            stdout=sys.stdout,
            stderr=sys.stdout)
        self.client = LSPClient(self.serverProc.stdin, self.serverProc.stdout)
    
    def tearDown(self) -> None:
        super().tearDown()
        self.serverProc.__exit__(None, None, None)
    
    def testInitialize(self):
        responseStr = self.client.send("initialize", {
            "processId": 1000,
            "rootUri": None,
            "capabilities": {}
        })

        response = json.loads(responseStr)
        self.assertEqual(response["result"], {
            "capabilities": {
                "semanticTokensProvider": {
                    "legend": {
                        "tokenTypes": ["enumMember", "variable"],
                        "tokenModifiers": []
                    },
                    "range": True,
                    "full": True
                }
            },
            "serverInfo": {
                "name": "allium-lsp",
                "version": "0.0.1"
            }
        })



if __name__ == "__main__":
    unittest.main(argv=sys.argv[:1])
