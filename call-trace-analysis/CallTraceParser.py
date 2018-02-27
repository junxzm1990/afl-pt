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
            text += e + ',' 
        return text
    def getHASH(self, text):
        hash_object = hashlib.md5(text)
        return hash_object.hexdigest()
    
    def printResult(self):
        for callset in self.res:
            print("==========%d=========" % callset.id)
            for key in callset.callChainSet.keys():
                #print ("HASH:%s" % key)  
                print ("length:%d" % (callset.callChainSet[key][0]))  
                print ("hit:%d" % (callset.callChainSet[key][1]))  
                #print ("call chain:%s" % (callset.callChainSet[key][2]))  
            print ("")
    def printMergedResult(self):
        for callset in self.res:
            print("==========%d=========" % callset.id)
            mergedSet={}
            for key in callset.callChainSet.keys():
                length = callset.callChainSet[key][0]  
                hit = callset.callChainSet[key][1]
                if length in mergedSet:
                    tmp = mergedSet[length]
                    mergedSet[length] = tmp + hit
                else:
                    mergedSet[length] = hit
            for key in mergedSet:
                print ("length:%d" % key)
                print ("hit:%d" % mergedSet[key])
            print ("")

    def printResultWithCallChain(self, funcMapParser):
        for callset in self.res:
            print("==========%d=========" % callset.id)
            for key in callset.callChainSet.keys():
                print ("HASH:%s" % key)  
                print ("length:%d" % (callset.callChainSet[key][0]))  
                print ("hit:%d" % (callset.callChainSet[key][1]))  
                print ("call chain:%s" % (self.getDetailedCallChain(callset.callChainSet[key][2], funcMapParser)))  
            print("")
    def getDetailedCallChain(self, callChainIdList, funcMapParser):
        fidList = callChainIdList.split(',')
        res = ""
        for fid in fidList:
            if fid.strip() != "":
                res += funcMapParser.getFuncName(fid) + ','
        return res
        

class FuncMapParser():
    """
    function: 
     parse fnmap.txt, which is given in the format like:
      Module Name: modulenamexxx
      func_name1:func_id1
      func_name2:func_id2
      ...
      Module Name: modulenameyyy
      func_name1:func_id11
      func_name2:func_id12
      ...
    output:
     build a hashmap (fid,[fname, module_id])
     build a hashmap (module_id, module_name)
    """
    def __init__(self, fname):
        self.fn = fname
        self.fnmap = {}
        self.momap = {}
        self.curModuleId = 0
        self.momap[0] = 'unknown_module'
        self.parseFuncMap()
        #print(self.fnmap)
        #print(self.momap)
    def parseFuncMap(self):
        with open(self.fn) as fp:
            line = fp.readline()
            while line:
                self.handleLine(line)
                line = fp.readline()
    def handleLine(self, line):
        fields = line.split(':')
        assert(len(fields) > 1)
        if fields[0] == 'Module Name':
            self.curModuleId += 1
            self.momap[self.curModuleId] = fields[1].strip()
        else:
            self.fnmap[fields[1].strip()] = (fields[0].strip(), self.curModuleId) 
    def checkOutFid(self, HASH):
        assert (HASH in self.fnmap.keys())
        fname = self.fnmap[HASH][0]
        modid = self.fnmap[HASH][1]
        assert (modid in self.momap.keys())
        moname = self.momap[modid]
        return (fname, moname)
    def getFuncName(self, HASH):
        return self.checkOutFid(HASH)[0]
    def getModuleName(self, HASH):
        return self.checkOutFid(HASH)[1]

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "require at least 1 parameter (calltrace list file)"
        sys.exit(-1)
    parser = CallTraceParser(sys.argv[1])
    #assert (os.path.isfile('fnmap.txt'))
    #fmapParser = FuncMapParser('fnmap.txt')
    parser.parseTrace()
    parser.printMergedResult()
    #parser.printResult()
    #parser.printResultWithCallChain(fmapParser)
