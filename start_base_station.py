from flask import Flask
from waitress import serve
from flask_app import create_app

HOST = '0.0.0.0'
PORT = 8080
WEB_APP = create_app()

print(f'Base station\'s flask app serving on http://{HOST}:{PORT}, visit http://localhost:{PORT} locally to view')
print(f'Type "ngrok http {PORT}" into command line to expose base station\'s flask app to internet')


serve(WEB_APP, host=HOST, port=PORT)
