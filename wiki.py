#!/bin/python3

from urllib.request import urlopen
from bs4 import BeautifulSoup
import re
import sys

URL = "https://en.wikipedia.org/wiki/Special:Random"

err = True
maxLen = 1000
minLen = 500

while err:
    try:
        err = False
        #URL = "https://en.wikipedia.org/wiki/Special:Random"
        try:
            source = urlopen(URL).read()
        except:
            exit()
        
        # send request
        #response = requests.get(URL)
        
        # parse response
        soup = BeautifulSoup(source, 'lxml')
        # get all attribute types from page
        #print(set([text.parent.name for text in soup.find_all(text=True)]))
        
        # extract plain text from paragraphs tagged with 'p'
        text = ''
        for paragraph in soup.find_all('p'):
            text += paragraph.text
            if len(text) > maxLen:
                break
        if len(text) < minLen or "help Wikipedia" in text:
            err = True
            continue
        
        #print(text)

        text = re.sub(r'\[[^\[\]]*\]+', '', text)
        text = re.sub(r'\s*\([^\(\)]*\)', '', text)
        text = re.sub(r'\n+', ' ', text)
        text = re.sub(r'\s+', ' ', text)
        text = text.strip()
        output = ''
        for m in re.finditer(r'[^.]*\.', text):
            if len(output) + len(m.group(0)) > maxLen:
                break
            output = output + m.group(0)
        print(output)
    except:
        err = True
