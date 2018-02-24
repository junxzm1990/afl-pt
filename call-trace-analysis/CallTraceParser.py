#!/usr/bin/env python
import hashlib
import sys
import os

class CallChainSet:
    def __init__(self, seedId):
        """
        map(HASH:CallChain)
        """
        self.id = seedId
        self.callChainSet = {}
    def insert(self, HASH, length, callChain):
        #print ("insert hash:%s, len:%d" % (HASH, length))
        if HASH in self.callChainSet:
            self.callChainSet[HASH][1] = self.callChainSet[HASH][1] + 1
        else:
            self.callChainSet[HASH] = []
            self.callChainSet[HASH].append(length)
            self.callChainSet[HASH].append(1) 
            self.callChainSet[HASH].append(callChain) 

class CallTraceParser:
    def __init__(self, callTraceFile):
        """
        TODO
        one example trace: fid,*,fid,fid,**
        """
        self.fn = callTraceFile
        self.res = []
    def parseTrace(self):
        with open(self.fn) as fp:
            line = fp.readline()
            cnt = 1
            while line:
                self.handleTraceLine(line, cnt)
                line = fp.readline()
                cnt += 1
    def handleTraceLine(self, line, cnt):
        # TODO
        ccs = CallChainSet(cnt)
        self.res.append(ccs)
        callStr = line.split(',')
        stack = []
        lastItem = ""
        for item in callStr:
            #print ("item:%s" % item)
            if item != "*":
                stack.append(item)
            elif item == "":
                continue
            else:
                if lastItem != "*":
                    text = self.getCallChain(stack)
                    HASH = self.getHASH(text)
                    length = len(stack)
                    ccs.insert(HASH, length, text)
                    assert len(stack) > 0
                    stack.pop()
                else:
                    assert len(stack) > 0
                    stack.pop()
            lastItem = item

    def getCallChain(self, stack):
        text = ""
        for e in stack:
            text += e 
        return text
    def getHASH(self, text):
        hash_object = hashlib.md5(text)
        return hash_object.hexdigest()
    
    def printResult(self):
        for callset in self.res:
            print("==========%d=========" % callset.id)
            for key in callset.callChainSet.keys():
                print ("HASH:%s" % key)  
                print ("length:%d" % (callset.callChainSet[key][0]))  
                print ("hit:%d" % (callset.callChainSet[key][1]))  
                print ("call chain:%s" % (callset.callChainSet[key][2]))  
            print ("")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "require at least 1 parameter (calltrace list file)"
        sys.exit(-1)
    parser = CallTraceParser(sys.argv[1])
    parser.parseTrace()
    parser.printResult()
