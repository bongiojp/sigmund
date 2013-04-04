import cx_Oracle
from random import randint
from random import choice
import sys,string,re,os,errno

class oracletestdb():
    def __init__(self, connectline):
        # conn / as sysdba
        # create user oracle identified by oracle;
        # grant create session to oracle;
        # ALTER USER oracle IDENTIFIED BY Passw0rd;
        # GRANT DBA TO oracle WITH ADMIN OPTION;
        self.con = cx_Oracle.connect(connectline)
        if (self.con):
            print "Successfully connected ..."
            print "Oracle Version:", self.con.version
            self.connected = True
        else:
            self.connected = False
    def close(self):
        if self.connected:
            self.con.close()
            self.connected = False
    def __del__(self):
        self.close()
    def isConnected(self):
        return self.connected
    def execute(self, queryline):
        cur = self.con.cursor()
        try:
            cur.execute(queryline)
        except Exception as e:
            cur.close()
            raise e
        cur.close()
    def executeandoutput(self,queryline):
        cur = self.con.cursor()
        cur.execute(queryline)
        return cur
    def droptablespace(self, tablespacename):
        execstr = 'select name from v$tablespace'
        cur = self.executeandoutput(execstr)
        regex = re.compile( r'\s*%s\s*'%tablespacename,re.M|re.I)
        for result in cur:
            matchobj = regex.match(result[0])
            if matchobj:
                execstr = 'drop tablespace {tsname} including contents'.format(tsname=tablespacename)
                self.execute(execstr)
        return
    def removetablespacefile(self, tablespacename):
        filename = "/mnt/temp/oracle/{tsname}.dbf".format(tsname=tablespacename)
        try: 
            os.remove(filename)
        except OSError as e:
            if  e.errno == errno.EEXIST:
                return True
        except Exception as e:
            print "Failed to remove file %s due to: %s"%(filename,e)
            return False
        return True
    def tablespaceexists(self, tablespacename):
        execstr = 'select name from v$tablespace'
        cur = self.executeandoutput(execstr)
        regex = re.compile( r'\s*%s\s*'%tablespacename,re.M|re.I)
        for result in cur:
            matchobj = regex.match(result[0])
            if matchobj:
                return True
        return False
    def createtablespace(self, tablespacename):
        execstr = 'create tablespace {tsname} datafile \'/mnt/temp/oracle/{tsname}.dbf\' size 10m'.format(tsname=tablespacename)
        self.execute(execstr)
    def createtable(self, tablespacename, tablename):
        execstr = '''CREATE TABLE {tname} (
         empno      NUMBER(5) PRIMARY KEY,
         ename      VARCHAR2(15) NOT NULL,
         ssn        NUMBER(9),
         job        VARCHAR2(10),
         mgr        NUMBER(5),
         hiredate   DATE DEFAULT (sysdate),
         photo      BLOB,
         sal        NUMBER(7,2),
         hrly_rate  NUMBER(7,2) GENERATED ALWAYS AS (sal/2080),
         comm       NUMBER(7,2),
         deptno     NUMBER(3) NOT NULL)
         TABLESPACE {tsname}
         STORAGE ( INITIAL 50K)'''.format(tsname=tablespacename, tname=tablename)
        self.execute(execstr)
    def randstr(self,length):
        # from http://stackoverflow.com/questions/2030053/random-strings-in-python
        return ''.join(choice(string.lowercase) for i in range(length))
    def insertrandom(self, tablename):
        empno = randint(1,99999)
        ename = '\'{s}\''.format(s=self.randstr(15))
        ssn = randint(10000000,99999999)
        job = '\'{s}\''.format(s=self.randstr(10))
        mgr = randint(1000,9999)
        sal = randint(10,10000)
        comm = randint(10,10000)
        deptno = randint(10, 99)
        execstr = "INSERT INTO {tname} (EMPNO,ENAME,SSN,JOB,MGR,SAL,COMM,DEPTNO) VALUES ({eno},{ena},{ss},{jo},{mg},{sa},{com},{dep})".format(tname=tablename,eno=empno,ena=ename,ss=ssn,jo=job,mg=mgr,sa=sal,com=comm,dep=deptno)
    # TODO: catch cx_Oracle.IntegrityError: ORA-00001: unique constraint (ORACLE.SYS_C0011027) violated
        try:
            self.execute(execstr)
        except cx_Oracle.IntegrityError as e:
            return True
        except Exception as e:
            return False
        return True


if __name__ == '__main__':
    db = oracletestdb('oracle/Passw0rd@127.0.0.1/orc')
    if not db.connected:
        sys.exit(1)

    # clean up tablespace if it already exists
    print "** Dropping tablespace 'testspace'"
    db.droptablespace('testspace')
    if db.tablespaceexists('testspace'):
        print "\tFailed to destroy tablespace 'testspace'"
        sys.exit(1)

    print "** Removing database file from filesystem"
    if not db.removetablespacefile('testspace'):
        print "\tFailed to remove tablespace file"
        sys.exit(1)

    print "** Creating tablespace 'testspace'"
    db.createtablespace('testspace')

    if db.tablespaceexists('testspace'):
        print "** Tablespace 'testspace' created successfully"
    else:
        print "\tFailed to create tablespace 'testspace'."
        sys.exit(1)

    print "** Creating table 'testtable' in tablespace 'testspace'"
    db.createtable('testspace', 'testtable')

    print "** Inserting 100 random rows into table 'testtable' in tablespace 'testspace'"
    for num in range(1,100):
        db.insertrandom('testtable')

    print "** Creating table 'testtable2' in tablespace 'testspace'"
    db.createtable('testspace', 'testtable2')

    print "** Inserting 100 random rows into table 'testtable2' in tablespace 'testspace'"
    for num in range(1,100):
        db.insertrandom('testtable')

    print "** Testing completed successfully, cleaning up database."
    print "** Removing tablespace 'testspace'"
    db.droptablespace('testspace')
    print "** Closing connection to database"
    db.close()
    sys.exit(0)
