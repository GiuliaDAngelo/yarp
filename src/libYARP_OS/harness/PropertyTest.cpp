// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2006 Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#include <yarp/os/Property.h>

#include <yarp/os/impl/UnitTest.h>
//#include "TestList.h"


#include <ace/OS_NS_stdlib.h>
// does ACE require new c++ header files or not?
#if ACE_HAS_STANDARD_CPP_LIBRARY
#include <fstream>
using namespace std;
#else
#include <fstream.h>
#endif


using namespace yarp::os::impl;
using namespace yarp::os;

class PropertyTest : public UnitTest {
public:
    virtual String getName() { return "PropertyTest"; }

    void checkPutGet() {
        report(0,"checking puts and gets");
        Property p;
        p.put("hello","there");
        p.put("hello","friend");
        p.put("x","y");
        checkTrue(p.check("hello"), "key 1 exists");
        checkTrue(p.check("x"), "key 2 exists");
        checkTrue(!(p.check("y")), "other key should not exist");
        checkEqual(p.find("hello").toString().c_str(),"friend", 
                   "key 1 has good value");
        checkEqual(p.find("x").toString().c_str(),"y", 
                   "key 2 has good value");
        p.fromString("(hello)");
        checkTrue(p.check("hello"), "key exists");
        Value *v;
        checkFalse(p.check("hello",v), "has no value");
    }


    void checkTypes() {
        report(0,"checking puts and gets of various types");
        Property p;
        p.put("ten",10);
        p.put("pi",(double)3.14);
        checkEqual(p.find("ten").asInt(),10,"ten");
        checkTrue(p.find("pi").asDouble()>3,"pi>3");
        checkTrue(p.find("pi").asDouble()<4,"pi<4");
        p.unput("ten");
        checkTrue(p.find("ten").isNull(),"unput");
    }


    void checkNegative() {
        report(0,"checking things are NOT found in correct manner");
        Property p;
        p.put("ten",10);
        checkTrue(p.check("ten"),"found");
        checkFalse(p.check("eleven"),"not found");
        Bottle& bot = p.findGroup("twelve");
        checkTrue(bot.isNull(),"group not found");
    }

    void checkExternal() {
        report(0,"checking external forms");
        Property p;
        p.fromString("(foo 12) (testing left right)");
        checkEqual(p.find("foo").asInt(),12,"good key 1");
        checkEqual(p.find("testing").asString().c_str(),"left","good key 2");
        checkEqual(p.findGroup("testing").toString().c_str(),
                   "testing left right","good key 2 (more)");

        Property p2;
        p2.fromString(p.toString().c_str());
        checkEqual(p.find("testing").asString().c_str(),"left","good key after copy");

        Property p3;
        char *args[] = {"CMD","--size","10","20","--mono","on"};
        p3.fromCommand(5,args);
        Bottle bot(p3.toString().c_str());
        checkEqual(bot.size(),2,"right number of terms");
        checkEqual(p3.findGroup("size").get(1).toString().c_str(),"10","width");
        checkEqual(p3.findGroup("size").get(2).toString().c_str(),"20","height");
        checkTrue(p3.findGroup("size").get(1).isInt(),"width type");
        checkEqual(p3.findGroup("size").get(1).asInt(),10,"width type val");

        report(0,"reading from config-style string");
        Property p4;
        p4.fromConfig("size 10 20\nmono on\n");
        Bottle bot2(p4.toString().c_str());
        checkEqual(bot2.size(),2,"right number of terms");
        checkEqual(p4.findGroup("size").get(1).toString().c_str(),"10","width");
        checkEqual(p4.findGroup("size").get(2).toString().c_str(),"20","height");
        checkTrue(p4.findGroup("size").get(1).isInt(),"width type");
        checkEqual(p4.findGroup("size").get(1).asInt(),10,"width type val");

        report(0,"more realistic config-style string");
        Property p5;
        p5.fromConfig("[cat1]\nsize 10 20\nmono on\n[cat2]\nfoo\t100\n");
        Bottle bot3(p5.toString().c_str());
        checkEqual(bot3.size(),2,"right number of terms");
        checkEqual(p5.findGroup("cat1").findGroup("size").get(1).asInt(),
                   10,"category 1, size, width");
        checkEqual(p5.findGroup("cat2").findGroup("foo").get(1).asInt(),
                   100,"category 2, foo");

        report(0,"command line style string");
        Property p6;
        char *strs[] = { "program", "--name", "/foo" };
        p6.fromCommand(3,strs);
        checkEqual(p6.find("name").asString().c_str(),"/foo",
                   "command line name");
        Value *v = NULL;
        p6.check("name",v);
        checkTrue(v!=NULL,"check method");

        Searchable *network = &p6.findGroup("NETWORK");
        if (network->isNull()) { network = &p6; }
        v = NULL;
        network->check("name",v);
        checkTrue(v!=NULL,"check method 2");

        Property p7;
    }

    void checkLineBreak() {
        report(0,"checking line break");
        Property p;
        p.fromConfig("x to\\\ny 20\n");
        checkFalse(p.check("y"),"ran on ok");
        checkEqual(p.findGroup("x").get(1).asString().c_str(),"toy","splice ok");
    }

    virtual void checkCopy() {
        report(0,"checking copy");
        Property p0;
        p0.fromString("(foo 12) (testing left right)");
        {
            Property p(p0);
            checkEqual(p.find("foo").asInt(),12,"good key 1");
            checkEqual(p.find("testing").asString().c_str(),"left",
                       "good key 2");
            checkEqual(p.findGroup("testing").toString().c_str(),
                       "testing left right","good key 2 (more)");
        }
        {
            Property p;
            p.fromString("bozo");
            p = p0;
            checkEqual(p.find("foo").asInt(),12,"good key 1");
            checkEqual(p.find("testing").asString().c_str(),"left",
                       "good key 2");
            checkEqual(p.findGroup("testing").toString().c_str(),
                       "testing left right","good key 2 (more)");
        }

    }


    virtual void checkExpansion() {
        report(0,"checking expansion");
        Property p;
        p.fromConfig("\
color red\n\
yarp1 $__YARP__\n\
yarp2 ${__YARP__}\n\
yarp3 pre_${__YARP__}_post\n\
");
        checkEqual(p.find("color").asString().c_str(),"red","normal key");
        checkEqual(p.find("yarp1").asInt(),1,"basic expansion");
        checkEqual(p.find("yarp2").asInt(),1,"expansion with parenthesis");
        checkEqual(p.find("yarp3").asString().c_str(),"pre_1_post",
                   "expansion with neighbor");

        Property env;
        env.put("TARGET","Earth");
        p.fromConfig("\
targ $TARGET\n\
",env);
        checkEqual(p.find("targ").asString().c_str(),"Earth",
                   "environment addition");

        p.fromConfig("\
x 10\n\
y 20\n\
check $x $y\n\
");
        checkEqual(p.findGroup("check").get(1).asInt(),10,"local x is ok");
        checkEqual(p.findGroup("check").get(2).asInt(),20,"local y is ok");
    }


    virtual void checkUrl() {
        report(0,"checking url parsing");
        Property p;
        p.fromQuery("prop1=val1&prop2=val2");
        checkEqual(p.find("prop1").asString().c_str(),"val1","basic prop 1");
        checkEqual(p.find("prop2").asString().c_str(),"val2","basic prop 2");
        p.fromQuery("http://foo.bar.org/link?prop3=val3&prop4=val4",true);
        checkEqual(p.find("prop3").asString().c_str(),"val3","full prop 3");
        checkEqual(p.find("prop4").asString().c_str(),"val4","full prop 4");
        p.fromQuery("prop1=val+one&prop2=val%2Ftwo%2C");
        checkEqual(p.find("prop1").asString().c_str(),"val one","mix prop 1");
        checkEqual(p.find("prop2").asString().c_str(),"val/two,","mix prop 2");
    }


    void checkNesting() {
        report(0,"checking nested forms");
        Property p;
        p.fromConfig("[sect a]\nhello there\n[sect b]\nx 10\n");
        ConstString sects = p.findGroup("sect").tail().toString();
        checkEqual(sects.c_str(),"a b","section list present");
        ConstString hello = p.findGroup("a").find("hello").asString();
        checkEqual(hello.c_str(),"there","individual sections present");
    }

    void checkComment() {
        report(0,"checking comments");
        Property p;
        p.fromConfig("x 10\n// x 11\n");
        checkEqual(p.find("x").asInt(),10,"comment ignored ok");
        p.fromConfig("url \"http://www.robotcub.org\"\n");
        checkEqual(p.find("url").asString().c_str(),"http://www.robotcub.org","url with // passed ok");        
    }

    virtual void checkWipe() {
        report(0,"checking wipe suppression");
        Property p;
        p.put("x",12);
        p.fromConfig("y 20",false);
        checkEqual(p.find("x").asInt(),12,"x is ok");
        checkEqual(p.find("y").asInt(),20,"y is ok");
    }

    virtual void checkBackslashPath() {
        // on windows, backslashes are used in paths
        // if passed on command-line, don't be shocked
        report(0,"checking backslash path behavior");
        Property p;
        ConstString target = "conf\\brains-brains.ini";
        char *argv[] = {
            "PROGRAM NAME",
            "--file",
            (char*)target.c_str()
        };
        int argc = 3;
        p.fromCommand(argc,argv);
        checkEqual(p.find("file").asString().c_str(),target.c_str(),
                   "string with slash");
    }


    virtual void checkIncludes() {
        report(0,"checking include behavior");

        const char *fname1 = "_yarp_regression_test1.txt";
        const char *fname2 = "_yarp_regression_test2.txt";

        // create some test files

        {
            ofstream fout1(fname1);
            fout1 << "x 1" << endl;
            fout1.close();
            ofstream fout2(fname2);
            fout2 << "[include " << fname1 << "]" << endl;
            fout2 << "y 2" << endl;
            fout2.close();
            Property p;
            p.fromConfigFile(fname2);
            checkEqual(p.find("x").asInt(),1,"x is ok");
            checkEqual(p.find("y").asInt(),2,"y is ok");
        }


        {
            ofstream fout1(fname1);
            fout1 << "x 1" << endl;
            fout1.close();
            ofstream fout2(fname2);
            fout2 << "[include base " << fname1 << "]" << endl;
            fout2 << "y 2" << endl;
            fout2.close();
            Property p;
            p.fromConfigFile(fname2);
            checkEqual(p.findGroup("base").find("x").asInt(),1,"x is ok");
            checkEqual(p.find("y").asInt(),2,"y is ok");
        }

        {
            ofstream fout1(fname1);
            fout1 << "x 1" << endl;
            fout1.close();
            ofstream fout2(fname2);
            fout2 << "[base]" << endl;
            fout2 << "w 4" << endl;
            fout2 << "[base]" << endl;
            fout2 << "z 3" << endl;
            fout2 << "[include base " << fname1 << "]" << endl;
            fout2 << "y 2" << endl;
            fout2.close();
            Property p;
            p.fromConfigFile(fname2);
            checkEqual(p.findGroup("base").find("x").asInt(),1,"x is ok");
            checkEqual(p.find("y").asInt(),2,"y is ok");
            checkEqual(p.findGroup("base").find("z").asInt(),3,"z is ok");
            checkEqual(p.findGroup("base").find("w").asInt(),4,"w is ok");
        }
    }


    virtual void checkCommand() {
        report(0,"checking command line parsing");
        char *argv[] = { 
            "program",
            "--on",
            "/server",
            "--cmd",
            "\"ls foo\""
        };
        int argc = 5;
        Property p;
        p.fromCommand(argc,argv);
        ConstString target1 = "(cmd \"ls foo\") (on \"/server\")";
        ConstString target2 = "(on \"/server\") (cmd \"ls foo\")";
        ConstString actual = p.toString();
        if (actual==target1) {
            checkEqual(actual.c_str(),target1.c_str(),"command ok");
        } else {
            checkEqual(actual.c_str(),target2.c_str(),"command ok");
        }
    }

    virtual void checkMonitor() {
        report(0,"checking monitoring");
    }

    virtual void runTests() {
        checkPutGet();
        checkExternal();
        checkTypes();
        checkNegative();
        checkCopy();
        checkExpansion();
        checkUrl();
        checkNesting();
        checkWipe();
        checkBackslashPath();
        checkIncludes();
        checkCommand();
        checkComment();
        checkLineBreak();
        checkMonitor();
    }
};

static PropertyTest thePropertyTest;

UnitTest& getPropertyTest() {
    return thePropertyTest;
}
