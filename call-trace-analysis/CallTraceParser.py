#!/usr/bin/env python
import hashlib
import sys
import os
import argparse
import progressbar

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
            lines = fp.readlines()
            num_of_lines = len(lines)
            bar = progressbar.ProgressBar(max_value=num_of_lines)
            bar_count = 0
            cnt = 1
            for line in lines:
                self.handleTraceLine(line, cnt)
                line = fp.readline()
                cnt += 1
                bar_count += 1
                bar.update(bar_count)
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
                    try:
                        assert len(stack) > 0
                        stack.pop()
                    except AssertionError:
                        sys.stderr.write("pop empty stack\n")
                else:
                    try:
                        assert len(stack) > 0
                        stack.pop()
                    except AssertionError:
                        sys.stderr.write("pop empty stack\n")
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
            print("=============%d===========" % (callset.id))
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
                print ("hit:%d" % AllMergedSet[key])
            print("")

    def printAllMergedResult(self):
        AllMergedSet = {}
        for callset in self.res:
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
                if key in AllMergedSet:
                    AllMergedSet[key] += mergedSet[key]
                else:
                    AllMergedSet[key] = mergedSet[key]

        # get percentage info
        totalHits = 0
        for key in AllMergedSet:
            totalHits += AllMergedSet[key]

        print("All merged results:")
        accHit = 0
        for key in AllMergedSet:
            print ("length:%d" % key)
            hit = AllMergedSet[key]
            accHit += hit
            per = hit * 1.0 / totalHits * 100
            accPer = accHit * 1.0 / totalHits * 100
            print ("hit:%.2f%% (%d/%d) accumulate hit:%.2f%% (%d/%d)" % (per, hit, totalHits, accPer, accHit, totalHits))

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
            lines = fp.readlines()
            num_of_lines = len(lines)
            bar = progressbar.ProgressBar(max_value=num_of_lines)
            bar_count = 0
            for line in lines:
                try:
                    self.handleLine(line)
                except AssertionError:
                    sys.stderr.write("handle Line error!\n")
                line = fp.readline()
                bar_count += 1
                bar.update(bar_count)
    def handleLine(self, line):
        fields = line.split(':')
        assert(len(fields) > 1)
        if fields[0] == 'Module Name':
            self.curModuleId += 1
            self.momap[self.curModuleId] = fields[1].strip()
        else:
            self.fnmap[fields[1].strip()] = (fields[0].strip(), self.curModuleId) 
    def checkOutFid(self, HASH):
        try:
            assert (HASH in self.fnmap.keys())
            fname = self.fnmap[HASH][0]
            modid = self.fnmap[HASH][1]
        except AssertionError:
            fname = "error! not found!"
            modid = 0
            sys.stderr.write("HASH:%s not in fnmap.keys()!\n" % HASH)
            #sys.stderr.write("HASH not in fnmap.keys()!\n")
        try:
            assert (modid in self.momap.keys())
            moname = self.momap[modid]
        except AssertionError:
            moname = "error! not found!"
            sys.stderr.write("modid not in momap.keys()!\n")
        return (fname, moname)
    def getFuncName(self, HASH):
        return self.checkOutFid(HASH)[0]
    def getModuleName(self, HASH):
        return self.checkOutFid(HASH)[1]

def parse_cmdline():

    p = argparse.ArgumentParser()

    p.add_argument("-m", "--merge-result", type=str, required=False,
            help="control whether or not to merge all results, Y means yes, N means no",
            default='Y')

    p.add_argument("-f", "--function-map", type=str, required=False,
            help="function hash to name map file")

    p.add_argument("-i", "--input-file", type=str, required=True,
            help="input call trace file")

    return p.parse_args()

def usage():
    print "Usage: python {0} -i calltrace.txt -m [YN] [-f fnmap.txt] " % sys.argv[0]
    print "do pip install progressbar2"

if __name__ == "__main__":
    cargs = parse_cmdline()
    parser = CallTraceParser(cargs.input_file)
    parser.parseTrace()

    if cargs.merge_result == "Y":
        parser.printAllMergedResult()
    else:
        parser.printMergedResult()

    # parse function name
    if cargs.function_map is not None:
        assert (os.path.isfile(cargs.function_map))
        fmapParser = FuncMapParser(cargs.function_map)
        parser.printResultWithCallChain(fmapParser)
