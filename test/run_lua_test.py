#!/usr/bin/env python2.7
import sys
import unittest
import char


def get_test_names(test_suite):
    names = []
    for test in test_suite:
        if isinstance(test, unittest.TestSuite):
            names += get_test_names(test)
        else:
            names.append(test.id().split(".")[2])
    return names

def test_all():
    tl = unittest.defaultTestLoader
    result = unittest.TestResult()
    ts = tl.discover("", "*_test.py")
    tr = unittest.TextTestRunner()
    tr.run(ts)

def coverage():
    types = {}
    tl = unittest.defaultTestLoader
    ts = tl.discover("", "*_test.py")
    tests =  get_test_names(ts)

    from test_setup import imm
    imm.open_luai()
    type_names = imm.luai_get_val("getluatype()")
    imm.close_luai()

    for name in type_names:
        imm.open_luai()
        types[name] = imm.luai_get_val("getluatype('{}')".format(name))
        imm.close_luai()

        for member_type in types[name]:
            for member in types[name][member_type]:
                test_name = "test_{}_{}_{}".format(
                        name, member_type, member['field'])
                if test_name not in tests:
                    print "No test for: " + test_name

    imm.open_luai()
    glob_names = imm.luai_get_val("getglobals()")
    imm.close_luai()

    for glob in glob_names:
        test_name = "test_glob_{}{}".format(
                 (glob['lib'] + "_") if 'lib' in glob else '',
                 glob['name'])
        if test_name not in tests:
            print "No test for: " + test_name
    

def generate():
    tl = unittest.defaultTestLoader

    ch = char.Immortal("vodur", "yumminess")
    ch.connect("rooflez.com", 7101)

    ch.open_luai()
    type_names = ch.luai_get_val("getluatype()")
    
    all_tests = unittest.TestSuite()

    for name in type_names:
        types[name] = ch.luai_get_val("getluatype('{}')".format(name))


        with open(name + "_test.py", "w") as f:
            f.write("import unittest\n")

            for member_type in ('get', 'set', 'method'):
                if len(types[name][member_type]) > 0:
                    f.write("\n\nclass {}(unittest.TestCase):\n".format(
                        member_type.capitalize()))
                for member in types[name][member_type]:
                    f.write("\n    def test_{}_{}_{}(self):\n".format(
                        name, member_type, member['field']))
                    f.write("        raise NotImplementedError\n")
    
    ch.close_luai()


def main():
    if len(sys.argv) < 2:
        test_all()
    elif sys.argv[1] == "coverage":
        coverage()


if __name__ == '__main__':
    main()
