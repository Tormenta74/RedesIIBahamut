#!/bin/env bash

# GET

echo "GET /"
curl 0.0.0.0:8000 --max-time 2

echo "GET /index.html"
curl 0.0.0.0:8000/index.html --max-time 2

echo "GET /amaia.jpg"
curl 0.0.0.0:8000/amaia.jpg --max-time 2 -o -

echo "GET /scripts/fahrenheit.py?temp=16.0"
curl 0.0.0.0:8000//scripts/fahrenheit.py?temp=16.0 --max-time 2

echo "GET /scripts/fahrenheit.py?scale=c"
curl 0.0.0.0:8000//scripts/fahrenheit.py?scale=c --max-time 2

echo "GET /scripts/fahrenheit.py?scale=c&temp=16.0"
curl 0.0.0.0:8000//scripts/fahrenheit.py?scale=c&temp=16.0 --max-time 2

echo "GET /scripts/fahrenheit.py?temp=16.0&scale=c"
curl 0.0.0.0:8000//scripts/fahrenheit.py?temp=16.0&scale=c --max-time 2

# POST

#echo "GET /"
#curl 0.0.0.0:8000
#
#echo "GET /index.html"
#curl 0.0.0.0:8000/index.html
#
#echo "GET /amaia.jpg"
#curl 0.0.0.0:8000/amaia.jpg
