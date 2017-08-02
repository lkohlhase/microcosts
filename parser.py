import pickle


#TODO reformat algorithm 2
#TODO change include stuff to include everything


#C++ style requirements: run it through file on: http://format.krzaq.cc/ (maybe also use cformatstandarlib or whatever it's called
#Known issues: Line breaks in for loops. Make sure there's no line breaks for looking nice in control statements
#Scope fuckery with things that should only be one line.
#for (stuff)
#for (more stuff)
#asdf=1
# for example. Dat shit don't fly.
#don't put any function stuff directly at the end of a file. Can lead to errors when we're checking if it's a prototype or not. TODO potentially fix this or decide to just document it. I think I fixed it, will have to check htough I guess
#No nested switch case at all.

class Parser:
    def __init__(self,filename,from_file=False):
        '''
        Initializes an instance of the parser, given the name of the initial file to be parsed. If from_file is set to true, loads filename from a log.
        :param filename: Name of the file to be analyzed

        '''
        if from_file:
            values=pickle.load(open(filename,'rb'))
            self.functionnames=values[0]
            self.functioncosts=values[1]
            self.files=values[2]
            self.rawfile=values[3]
            self.functions=values[4]
            self.evaluated=values[5]
        else:
            spliterino=filename.split('/')
            actualfilename=spliterino[-1]
            path=[x+"/" for x in spliterino[:-1]]
            path="".join(path)
            helpfile=self.readin(filename)
            files=[]
            file=[]
            filerinos=[(actualfilename,helpfile)]
            for i in range(10):
                newfiles=[]
                for (filename2,filebody) in filerinos:
                    newfiles+=[filename2]
                    newfiles+=self.find_includes(filebody)
                newfiles=list(set(newfiles))

                filerinos=[]
                for name in newfiles:
                    filerinos.append((name,self.readin(path+name)))
            self.files=newfiles
            self.rawfile=[x[1] for x in filerinos]
            print('We are analyzing these files:')
            print(self.files)
            functionerinos=[]
            for i in filerinos:
                functionerinos+=self.find_functions(i[1],i[0])
            self.functions=functionerinos
            self.functioncosts = []
            self.functionnames = []
            for x in self.functions:
                self.functionnames.append(x[0]) #Names of all analyzed functions
            for x in self.functions:
                self.functioncosts.append(self.basic_parse(x))


    def evaluate(self):
        evaluated=[]
        for i in self.functioncosts:
            evaluated.append(self.evaluate_object(i))
        for j in range(10): #We just run through this 10 times, to make sure that all the interactions work out
            for i in range(len(evaluated)):
                evaluated[i]=self.evaluate_object(evaluated[i])
            self.functioncosts=evaluated
        self.evaluated=evaluated
    def readin(self,filename):
        '''
        Reads in the raw lines from filename. Strips away whitespace too
        :param filename: Name of file to be read in
        :return: A list consisting of the lines with whitespace stripped
        '''
        try:
            f = open(filename, 'rU')
            file = []
            for x in f:
              file.append(x.strip())
            return file
        except:
            print("Couldn't open file "+filename+". Reporting this as empty instead")
            return [""]
    def find_includes(self,file):
        '''
        Returns all relevant files to be included from the original file
        '''
        files = []
        for i in range(len(file)):
            x = file[i]
            splitline = x.split(" ")
            if 'include' in x:
                if '"' in splitline[1]:
                    split2 = splitline[1].split('"')
                    files.append(split2[1])
        return files

    def find_functions(self,filebody,filename=''):
        '''
        Finds the functions contained in raw text. Returns a list of tuples (functionname,functionbody)
        :return: A list of tuples (functionname, functionbody)
        '''
        functions = []
        file=filebody
        for i in range(len(file)):
            x = file[i]
            splitline = x.split(" ")
            if splitline[0]=='static':
                splitline=splitline[1:]
            if splitline[0] in ["int", 'void', 'double','float','uint16_t','uint32_t','enum','uint8_t','int8_t','in16_t','int32_t']: #TODO check whether other functions are needed
                if ("(" in x and not "=" in x):  # It's actually a function
                    type = splitline[0]
                    rest = ''.join(splitline[1:])
                    rest = rest.split('(')
                    name = rest[0]
                    body = self.function_scope(file[i + 1:])
                    if body != "prototype":
                        functions.append((name, body,filename))

        return functions


    def print_result(self):
        '''
        prints the final cost of the evaluated functions
        :return: same thign
        '''
        for function in self.evaluated:
            print(function['name'])
            print(function['finalcost'])


    def function_scope(self,array):
        '''
        Takes the raw files as input, starting at the point where we want to find a function scope from, and returns a guess at what the scope is (indicated by {} or just one line). Anything not in braces(which must be in separate lines), or just one line, does not get identified.
        :param array: List of code lines, starting at the function statement for which we are looking for scope for
        :return: Scope of the statement we are looking for scope for. List of function lines
        '''
        content=[]
        if "{" not in array[0]: #There's no body of the function, so it has to be a prototype
            return "prototype"
        else:
            i=1
            bracecounter=1
            while(bracecounter>0):
                bracecounter+=array[i].count("{")
                bracecounter-=array[i].count("}")
                content.append(array[i])
                i=i+1
            return content


    def basic_parse(self,function):
        '''
        Takes a function as found by find_function() and returns a very basic parse of it. Only counts actions immediately inside the function scope, and doesn't evaluate of other functions included inside etc.
        :param function: A tuple consisting of (functionname,functionbody)
        :return: A dict containing a basic parse.
        '''
        function_index=self.functionnames #This used to not be in a class, so I'm keeping legacynames
        if ('random' not in function[0]):
             print('parsing '+function[0]+' in '+function[2])
        functionlist=function[1]
        object={}
        object['name']=function[0]
        object['in file']=function[2]
        object['costs']={}
        object['costs']['baseline']={} #Actions immediately included in the function.
        object['costs']['baseline']['amount']=1
        object['costs']['baseline']['varassignment'] =0
        object['costs']['baseline']['addition'] =0
        object['costs']['baseline']['addition'] =0
        object['costs']['baseline']['multiplication'] =0
        object['costs']['baseline']['division'] = 0
        object['costs']['baseline']['bitshift'] = 0
        object['costs']['baseline']['return'] = 0
        object['costs']['baseline']['incrementation']=0
        object['costs']['other_functions']={} #Other functions included inside the functionbody
        object['costs']['if']=[] #Essentially a list of objects
        object['costs']['for']=[]
        object['costs']['switch']=[]
        object['costs']['while']=[]
        for y in range(len(functionlist)):
            x=functionlist[y]
            if '//' in x:
                x=x.split('//')[0]#getting rid of single line comments
            #Now we do some if/for handling
            if ('if (' in x):
                object['costs']['if'].append({})
                object['costs']['if'][-1]['condition']=x
                object['costs']['if'][-1]['options']=[]
                iterator=y
                #First we check the scope
                notdone=True
                while(notdone): #We need a more complicated structure than for for, because we potentially need a bunch of different scopes, since arbitrary else can be added
                    if iterator<len(functionlist):
                        functionlist[iterator]='' #We're deleting all traces of if. This is so that some else if isn't later matched onto

                        iterator+=1

                        scope=[]
                        if ('{' in functionlist[iterator]): #it's a proper scoped thingy
                            bracecounter=1

                            functionlist[iterator]='' #We delete the closing } later, so for later scope change we need to do this.
                            iterator+=1
                            while(bracecounter>0): #Finding the elements
                                if iterator < len(functionlist):
                                    bracecounter += functionlist[iterator].count("{")
                                    bracecounter -= functionlist[iterator].count("}")
                                    scope.append(functionlist[iterator])
                                    functionlist[iterator]="" #We delete the contents that we will check later
                                    iterator += 1
                                else:
                                    break

                            if iterator<len(functionlist):
                                if ('else' not in functionlist[iterator]):
                                    notdone=False
                        else:
                            scope=functionlist[iterator] #This is the next line
                            functionlist[iterator]='' #Remove the line so it doesn't get double counted
                            if ('else' not in functionlist[iterator+1]):
                                notdone=False
                        ifname=function[0]+' if: '+str(len(object['costs']['if'])) + ' option: '+ str(len(object['costs']['if'][-1]['options'])+1)
                        object['costs']['if'][-1]['options'].append(self.basic_parse((ifname,scope,function[2])))
                    else:
                        break
                    #We don't delete the if condition, since it will only happen once
            if ('switch (' in x):
                object['costs']['switch'].append({})
                object['costs']['switch'][-1]['condition']=x
                object['costs']['switch'][-1]['cases']=[]
                body=[]
                iterator=y+2 #First line after y is going to be consist just of {, so might as well skip that
                bracecounter=1
                while(bracecounter>0):
                    if iterator<len(functionlist):
                        bracecounter += functionlist[iterator].count("{")
                        bracecounter -= functionlist[iterator].count("}")
                        body.append(functionlist[iterator])
                        functionlist[iterator] = ""  # We delete the contents that we will check later
                        iterator += 1
                    else:
                        break
                casebody=[]
                casecounter=0
                for line in body:
                    if casecounter==0:
                        ifname = function[0] + ' switch: ' + str(len(object['costs']['switch'])) + ' case: ' + str(len(object['costs']['switch'][-1]['cases']) + 1)
                        if not casebody==[]:
                            object['costs']['switch'][-1]['cases'].append(self.basic_parse((ifname, casebody, function[2])))
                        casebody = []
                    if 'case' in line:
                        casecounter+=1
                    if 'break;' in line:
                        casecounter-=1
                    casebody.append(line)






            if ('for (' in x): #We're assuming every for loop has the form for(initialization;condition; incrementation). Any extra spaces and such will mess it up.
                object['costs']['for'].append({})
                object['costs']['for'][-1]['conditions']='t'+x[1:] # We change for into tor, just so we don't trigger on it again, when we later basic_parse the conditions.

                body=[]
                iterator=y+1 #This is the line where we expect a brace
                if ("{" not in functionlist[iterator]): #The body of the for loop is only one line long
                    body.append(functionlist[iterator])
                    functionlist[iterator]=""
                else:
                    bracecounter=1
                    iterator+=1
                    while(bracecounter>0):
                        if iterator < len(functionlist):
                            bracecounter+=functionlist[iterator].count('{')
                            bracecounter-=functionlist[iterator].count('}')
                            body.append(functionlist[iterator])
                            functionlist[iterator]=''
                            iterator+=1
                        else:
                            break
                forname=function[0]+' for: '+str(len(object['costs']['for']))
                object['costs']['for'][-1]['bodycost']=self.basic_parse((forname,body,function[2]))
                object['costs']['for'][-1]['optioncost']=self.basic_parse(('for conditions',[object['costs']['for'][-1]['conditions']],function[2]))
                functionlist[y]=""#We delete the contents of this line, since it will be evaluated later
                x="" #Also have to wipe x
            if (x=='do'): #this should work for do while, since it seems to only have do solo on the line
                object['costs']['while'].append({})
                body=[]
                iterator=y+2
                bracecounter=1
                while(bracecounter>0):
                    if iterator < len(functionlist):
                        bracecounter += functionlist[iterator].count('{')
                        bracecounter -= functionlist[iterator].count('}')
                        body.append(functionlist[iterator])
                        functionlist[iterator] = ''
                        iterator += 1
                    else:
                        break
                object['costs']['while'][-1]['conditions']='v'+body[-1][3:]
                whilename = function[0] + 'do while: ' + str(len(object['costs']['while']))
                object['costs']['while'][-1]['bodycost'] = self.basic_parse((whilename, body[:-1],function[2]))
                object['costs']['while'][-1]['optioncost'] = self.basic_parse(('do while conditions', [object['costs']['while'][-1]['conditions']],function[2]))
            if ('while (' in x):
                object['costs']['while'].append({})
                object['costs']['while'][-1]['conditions']='v'+x[1:] #vhile, so we don't match onto it again
                body=[]
                iterator=y+1
                if ('{' not in functionlist[iterator]):
                    body.append(functionlist[iterator])
                    functionlist[iterator]=""
                else:
                    bracecounter=1
                    iterator+=1
                    while(bracecounter>0):
                        if iterator < len(functionlist):
                            bracecounter+=functionlist[iterator].count('{')
                            bracecounter-=functionlist[iterator].count('}')
                            body.append(functionlist[iterator])
                            functionlist[iterator]=''
                            iterator+=1
                        else:
                            break
                whilename=function[0]+' while: '+str(len(object['costs']['while']))
                object['costs']['while'][-1]['bodycost']=self.basic_parse((whilename,body,function[2]))
                object['costs']['while'][-1]['optioncost']=self.basic_parse(('while conditions',[object['costs']['while'][-1]['conditions']],function[2]))
                functionlist[y]=""#We delete the contents of this line, since it will be evaluated later
                x="" #Also have to wipe x
            object['costs']['baseline']['varassignment']+=x.count("=")-2*x.count('==')-x.count('>=')-x.count('<=')
            object['costs']['baseline']['addition']+=x.count('+')-2*x.count('++')
            object['costs']['baseline']['addition']+=x.count('-')-2*x.count('--')
            object['costs']['baseline']['multiplication']+=x.count('*')-x.count('/*')-x.count('*/')
            object['costs']['baseline']['division']+=x.count('/')-2*x.count('//')-x.count('/*')-x.count('*/') #Need to remove comments.
            object['costs']['baseline']['bitshift']+=x.count('>>')+x.count('<<')
            object['costs']['baseline']['incrementation']+=x.count('++')+x.count('--')
            object['costs']['baseline']['return']+=x.count('return')
            for i in function_index:
                if i+'(' in x: # If we detect that there are other_functions being invoked
                    if i in object['costs']['other_functions']:
                        object['costs']['other_functions'][i]+=1
                    else:
                        object['costs']['other_functions'][i]=1



        return object

    def find_function(self,name):
        '''
        Takes a function name and returns the corresponding object
        :param name: Name of the function
        :return: The dictionary containing information about the functions cost
        '''
        objectlist=self.functioncosts
        for x in objectlist:
            if (x['name'] == name):
                return x
        print('Function ' + name + ' not found.')
    def evaluate_object(self,object):
        '''
        Takes a dictionary generated by basic_parse, and evaluates if, other functions and for loops
        :param object: dictionary generated by basic_parse
        :return: dictionary object with 'finalcost' added
        '''
        #Here we start adding the other_functions in.
        objectlist2=self.functioncosts
        object2=object
        object2['finalcost']={}
        object2['finalcost']['varassignment']=object['costs']['baseline']['varassignment']
        object2['finalcost']['addition']=object['costs']['baseline']['addition']
        object2['finalcost']['multiplication']=object['costs']['baseline']['multiplication']
        object2['finalcost']['division']=object['costs']['baseline']['division']
        object2['finalcost']['bitshift']=object['costs']['baseline']['bitshift']
        object2['finalcost']['return']=object['costs']['baseline']['return']
        object2['finalcost']['incrementation']=object['costs']['baseline']['incrementation']
        object2['finalcost']['if']=0
        for x in object['costs']['other_functions'].keys():
            foundfunction=self.find_function(x)
            if 'finalcost' in foundfunction:
                object2['finalcost']['varassignment']+=foundfunction['finalcost']['varassignment']*object['costs']['other_functions'][x] #Add the amount of operation in that new function, multiplied by however many times that function appears
                object2['finalcost']['addition'] += foundfunction['finalcost']['addition'] * object['costs']['other_functions'][x]
                object2['finalcost']['multiplication'] += foundfunction['finalcost']['multiplication'] * object['costs']['other_functions'][x]
                object2['finalcost']['division'] += foundfunction['finalcost']['division'] * object['costs']['other_functions'][x]
                object2['finalcost']['bitshift'] += foundfunction['finalcost']['bitshift'] * object['costs']['other_functions'][x]
                object2['finalcost']['return'] += foundfunction['finalcost']['return'] * object['costs']['other_functions'][x]
                object2['finalcost']['if']+=foundfunction['finalcost']['if']*object['costs']['other_functions'][x]
                object2['finalcost']['incrementation']+=foundfunction['finalcost']['incrementation']*object['costs']['other_functions'][x]
            else: #Now we do the same thing, except we have to look in baseline instead
                object2['finalcost']['varassignment']+=foundfunction['costs']['baseline']['varassignment']*object['costs']['other_functions'][x]
                object2['finalcost']['addition'] += foundfunction['costs']['baseline']['addition'] * object['costs']['other_functions'][x]
                object2['finalcost']['multiplication'] += foundfunction['costs']['baseline']['multiplication'] * object['costs']['other_functions'][x]
                object2['finalcost']['division'] += foundfunction['costs']['baseline']['division'] * object['costs']['other_functions'][x]
                object2['finalcost']['bitshift'] += foundfunction['costs']['baseline']['bitshift'] * object['costs']['other_functions'][x]
                object2['finalcost']['return'] += foundfunction['costs']['baseline']['return'] * object['costs']['other_functions'][x]
                object2['finalcost']['incrementation']+=foundfunction['costs']['baseline']['incrementation'] * object['costs']['other_functions'][x]
        #Now we do if
        for x in object['costs']['if']:
            options=[]
            object2['finalcost']['if']+=x['condition'].count('&&')
            object2['finalcost']['if']+=x['condition'].count('||')
            for j in range(len(x['options'])):
                i=x['options'][j]
                object2['finalcost']['if'] += 1  #Every option is at least one if
                evaluated_option=self.evaluate_object(i)
                options.append(evaluated_option)
                x['options'][j]=evaluated_option
            maxoption={}
            for i in object2['finalcost']:#Should be the keys
                maxoption[i]=options[0]['finalcost'][i]
            for i in options:
                for y in object2['finalcost']:
                    if maxoption[y]<i['finalcost'][y]:
                        maxoption[y]=i['finalcost'][y]#We go through all keys, and just select the biggest one
            for i in maxoption:
                object2['finalcost'][i]+=maxoption[i]
        for x in object['costs']['switch']:
            options=[]
            object2['finalcost']['if']+=1
            for j in range(len(x['cases'])):
                i =x['cases'][j]
                object2['finalcost']['if']+=1
                evaluated_option=self.evaluate_object(i)
                options.append(evaluated_option)
                x['cases'][j]=evaluated_option
            maxoption={}
            for i in object2['finalcost']:
                maxoption[i]=options[0]['finalcost'][i]
            for i in options:
                for y in object2['finalcost']:
                    if maxoption[y]<i['finalcost'][y]:
                        maxoption[y]=i['finalcost'][y]
            for i in maxoption:
                object2['finalcost'][i]+=maxoption[i]
        #Now we do for. Arguably hardest part here
        for x in object['costs']['for']:
            bodycost=self.evaluate_object(x['bodycost'])
            optioncost=self.evaluate_object(x['optioncost']) #potentially oversells regarding '='
            conditions=x['conditions']
            if 'numloops' not in x: #once numloops has been set once, it doesn't get changed anymore. If it was possible to automatically find it, we would have found it already, if we asked the user, the answer won't change.
                asdf=object['name']
                numloops,defaultstatus=self.for_condition(conditions,object['in file'],asdf)
                x['numloops']=numloops
                x['default']=defaultstatus
            x['finalcost']={}
            for key in object2['finalcost']:#goes through all the keys that we need for
                x['finalcost'][key]=x['numloops']*(bodycost['finalcost'][key]) #We add the optioncost manually, since stuff in there can be weird
            finaloptioncost={}
            for key in optioncost['finalcost']:
                finaloptioncost[key]=x['numloops']*optioncost['finalcost'][key]

            finaloptioncost['varassignment']=x['conditions'].count('=')-2*conditions.count('==')-conditions.count('<=')-conditions.count('>=') #that should catch all possible extra sources of =
            finaloptioncost['if']=x['numloops']*(1+conditions.count('&&')+conditions.count('||')) #The condition gets checked as many times as the loop runs.
            for key in object2['finalcost']:
                x['finalcost'][key]+=finaloptioncost[key]
                object2['finalcost'][key]+=x['finalcost'][key]

        for x in object['costs']['while']:
            bodycost=self.evaluate_object(x['bodycost'])
            optioncost=self.evaluate_object(x['optioncost'])
            conditions=x['conditions']
            if 'numloops' not in x:
                asdf=object['name']
                numloops,defaultstatus=self.while_error(conditions,object['in file'],object['name'])
                x['numloops']=numloops
                x['default']=defaultstatus
            x['finalcost']={}
            for key in object2['finalcost']:
                x['finalcost'][key]=x['numloops']*(bodycost['finalcost'][key])
            finaloptioncost={}
            for key in optioncost['finalcost']:
                finaloptioncost[key]=x['numloops']*optioncost['finalcost'][key]
            finaloptioncost['varassignment'] = x['conditions'].count('=') - 2 * conditions.count('==') - conditions.count('<=') - conditions.count('>=')  # that should catch all possible extra sources of =
            finaloptioncost['if'] = x['numloops'] * (1 + conditions.count('&&') + conditions.count('||'))  # The condition gets checked as many times as the loop runs.
            for key in object2['finalcost']:
                x['finalcost'][key] += finaloptioncost[key]
                object2['finalcost'][key] += x['finalcost'][key]
        return object2

    def removedefaults(self):
        '''
        Looks at all for loops etc. that are flagged as having default number of loops, then updates them.
        '''
        for object in self.functioncosts:
            self.removedefault(object)
    def removedefault(self,object):
        '''
        Helper function for removedefaults. Takes an object and removes the default from them.
        :param object: Object to be passed
        '''
        print('Removing defaults from '+object['name']+ ' in '+ object['in file'])
        for x in object['costs']['for']:
            if x['default']=='yes':
                condition=x['conditions']
                newnumloops,defaultstatus=self.for_error(condition,object['in file'],object['name'])
                x['default']=defaultstatus
                x['numloops']=newnumloops
                self.removedefault(x['bodycost'])#This is a fully fledged object as well
                self.removedefault(x['optioncost']) #If you have a for loop in your options, you are messed up. Don't do this. Nevertheless, we handle it


        for x in object['costs']['if']: #Check if there's any for loops in if
            for i in x['options']:
                self.removedefault(i)


    def for_condition(self,conditions,filename,functionname):
        '''
        Attempts to parse a line containing a for loop and figure out how many times it loops.
        :param conditions: string containing for loop e.g. for(i=0; i<5; i++)
        :return: Amount of repetitions of the for loop
        '''
        default=10
        splitted=conditions.split( ';')
        var_dec=splitted[0]
        condition=splitted[1] #We assume that there is just a single condition. Else it gets too difficult.
        increment=splitted[2]
        increasing=True
        modifier=0
        limit2=0 #Upper limit
        limit1=0 #Lower limit
        if '--' in increment:
            increasing=False
        if increasing:
            if '<=' in condition:
                modifier=1
                splitagain=condition.split('<=')
            else:
                modifier=0
                splitagain=condition.split('<')

            final=splitagain[-1]

            if is_number(final):
                limit2=float(final)


            else: #It's smaller than a variable
                return self.for_error(conditions,filename,functionname)
        else:
            if '>=' in condition:
                modifier=1
                splitagain=condition.split('>=')
            else:
                modifier=0
                splitagain=condition.split('>')
            final=splitagain[-1]
            if is_number(final):
                limit2=float(final)
            else:
                return self.for_error(conditions,filename,functionname)
        if '=' in var_dec:
            firstsplit=var_dec
            first=firstsplit[-1]
            if is_number(first):
                limit1=float(first)
            else:
                return self.for_error(conditions, filename,functionname)
        else:
            return self.for_error(conditions,filename,functionname)
        length=limit2-limit1+modifier
        return length, 'no'


    def for_error(self,condition,filename,functionname):
        '''
        For use when we can't figure out how many loops,so we ask the user.
        :param condition:  string containing for loop e.g. for(i=0; i<5; i++)
        :return: Amount of loops, as suggested by the user
        '''
        default=5
        print('There was an issue resolving the conditions of a for loop in | '+functionname+' | in file '+filename)
        print(condition)
        userinput=raw_input('What is the maximum amount of loops? ')
        if not is_number(userinput):
            print('Could not understand input, will use default value '+ str(default))
            return (default,'yes')
        return (int(userinput),'no')
    def while_error(self,condition,filename,functionname):
        '''
        For use when we can't figure out how many loops,so we ask the user.
        :param condition:  string containing for loop e.g. for(i=0; i<5; i++)
        :return: Amount of loops, as suggested by the user
        '''
        default=5
        print('There was an issue resolving the conditions of a while loop in | '+functionname+' | in file '+filename)
        print(condition)
        userinput=raw_input('What is the maximum amount of loops? ')
        if not is_number(userinput):
            print('Could not understand input, will use default value '+ str(default))
            return (default,'yes')
        return (int(userinput),'no')
    def save(self,filename):
        '''

        :param filename:
        :return:
        '''
        dumperino=[self.functionnames,self.functioncosts,self.files,self.rawfile,self.functions,self.evaluated]
        pickle.dump(dumperino,open(filename,'wb'))



def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


testparser=Parser('withdefaults.txt',from_file=True)
testparser.evaluate()
#testparser.save('withdefaults.txt')
testparser.removedefaults()
testparser.save('nodefaults.txt')
print(testparser.functionnames)
testparser.print_result()




#
# def loaderinobullshit(filename):
#     with open ('text.txt','wb') as handle:
#         pickle.dump(asdf[3],handle)
#     with open('text.txt','rb') as handle:
#         b=pickle.loads(handle.read())
#     for i in b:
#         print(i)

