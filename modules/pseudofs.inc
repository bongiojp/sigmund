#!/bin/bash

DIRNUMBER=1000
PREFIX=p
PSEUDOFS_SETUP=1

############################ TEST FUNCTIONS ############################

### setup_pseudofs: Setup the export config on Ganesha server 
SPEED[setup_pseudofs]=fast
TEST_TYPE[setup_pseudofs]=dev,admin,prod
NEEDS_ROOT[setup_pseudofs]=yes
function setup_pseudofs
{
    # setup only once
    if (($PSEUDOFS_SETUP == 0)); then
        newexportconfig=$EXPORTFILE.pseudofstest

        # copy export config from server
        scp root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE $BUILD_TEST_DIR/
        cp $BUILD_TEST_DIR/$EXPORTFILE $BUILD_TEST_DIR/$newexportconfig

        # generate new exports and append to server's export config
        $BUILD_TEST_DIR/ganesha_configs/createmanyexports.pl \
            $FS_DIR \
            $PREFIX \
            $GANVERSION \
            $DIRNUMBER \
            $FSAL >> $BUILD_TEST_DIR/$newexportconfig

        # copy export config to server
        scp $BUILD_TEST_DIR/$newexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
        
        # restart server
        ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"
    fi
}

### test_wr: check if writes in a pseudofs directory is allowed. Should return an error.
SPEED[cleanup_pseudofs]=fast
TEST_TYPE[cleanup_pseudofs]=dev,admin,prod
NEEDS_ROOT[cleanup_pseudofs]=yes
function cleanup_pseudofs
{
    # cleanup only once
    if (($PSEUDOFS_SETUP == 1)); then
        PSEUDOFS_SETUP=0
        # make sure previous export config is available
        # then copy previous export config to server
        if [ -e "$BUILD_TEST_DIR/$EXPORTFILE" ]
            scp $BUILD_TEST_DIR/$EXPORTFILE root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
        else
            echo error "Could not reinstate previous export config on server!!"
        fi

        # restart server
        ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"
    fi
}

### test_wr: check if writes in a pseudofs directory is allowed. Should return an error.
SPEED[test_wr]=fast
TEST_TYPE[test_wr]=dev,admin,prod
NEEDS_ROOT[test_wr]=yes
PRE_TEST[test_wr]=setup_pseudofs
function test_wr
{
    # create user directory
    file="$TEST_ROOT_DIR/test_wr"
    touch $file
    (! $?) || error "we should not be able to create a file in a pseudofs directory: $file"

    file="$TEST_ROOT_DIR/many"
    touch $file
    (! $?) || error "we should not be able to modify a pseudofs node: $file"
}

### test_rd: 
SPEED[test_rd]=fast
TEST_TYPE[test_rd]=dev,admin,prod
NEEDS_ROOT[test_rd]=yes
PRE_TEST[test_rd]=setup_pseudofs
function test_rd
{
    # create user directory
    file="$TEST_ROOT_DIR/pseudofs_test"
    cat $file
    (! $?) || error "$file doesn't exist, but we could read it in a pseudo fs directory!"

    file="$TEST_ROOT_DIR/many"
    cat $file
    (! $?) || error "$file does exist, but we shouldn't be able to read it in a pseudo fs directory!"
}

### test_rm: 
SPEED[test_rm]=fast
TEST_TYPE[test_rm]=dev,admin,prod
NEEDS_ROOT[test_rm]=yes
PRE_TEST[test_rm]=setup_pseudofs
function test_rm
{
    # rm pseudofs node
    file="$TEST_ROOT_DIR/pseudofs_test"
    rm $file
    (! $?) || error "$file doesn't exist, but we could remove it: $file"

    file="$TEST_ROOT_DIR/many"
    rm $file
    (! $?) || error "$file does exist, but we shouldn't be able to remove a pseudo fs node!"
}

### test_lookup: 
SPEED[test_lookup]=fast
TEST_TYPE[test_lookup]=dev,admin,prod
NEEDS_ROOT[test_lookup]=yes
PRE_TEST[test_lookup]=setup_pseudofs
function test_lookup
{
    # lookup pseudofs node
    file="$TEST_ROOT_DIR/pseudofs_test"
    ls $file
    (! $?) || error "$file doesn't exist, but we a lookup succeeded: $file!"

    file="$TEST_ROOT_DIR/many/$PREFIX1"
    ls $file
    ($?) || error "$file does exist and a lookup should have succeeded!"
}

### test_readdir: 
SPEED[test_readdir]=fast
TEST_TYPE[test_readdir]=dev,admin,prod
NEEDS_ROOT[test_readdir]=yes
PRE_TEST[test_readdir]=setup_pseudofs
function test_readdir
{
    # create user directory
    dir="$TEST_ROOT_DIR/many/"
    ls -l $dir
    ($?) || error "$dir does exist with $DIRNUMBER entries and a readdir should have succeeded!"

    #    ls -l $dir | wc -l
    # check if proper number of entries returned
}

### test_ls_junction: 
SPEED[test_ls_junction]=fast
TEST_TYPE[test_ls_junction]=dev,admin,prod
NEEDS_ROOT[test_ls_junction]=yes
PRE_TEST[test_ls_junction]=setup_pseudofs
function test_ls_junction
{
    # create user directory
    file="$TEST_ROOT_DIR/many/$PREFIX1"
    ls $file
    ($?) || error "$file does exist, is a junction to a real FS, and a ls should have succeeded!"
}

### test_check_inode: 
SPEED[test_check_inode]=fast
TEST_TYPE[test_check_inode]=dev,admin,prod
NEEDS_ROOT[test_check_inode]=yes
PRE_TEST[test_check_inode]=setup_pseudofs
function test_check_inode
{
    # First get the inode number from NFS
    file="$TEST_ROOT_DIR/many/$PREFIX10"
    inode1=`stat -c "%i" $file`

    # Second get the inode number from server local filesystem
    inode2=`ssh -tt root@$SERVER stat -c "%i" ${FS_DIR}/${PREFIX}10`

    # Now compare
    if (inode1 != inode2)
        ($?) || error "The inode in the server's local FS doesn't match inode seen in "
    fi

}

### test_remove_lookup: 
SPEED[test_remove_lookup]=fast
TEST_TYPE[test_remove_lookup]=dev,admin,prod
NEEDS_ROOT[test_remove_lookup]=yes
PRE_TEST[test_remove_lookup]=setup_pseudofs
function test_remove_lookup
{
    prevexportconfig=$EXPORTFILE.fromremovelookup
    newexportconfig=$EXPORTFILE.pseudofstest
    removedexport=10

    # first have client cache result of pseudofs node
    file="$TEST_ROOT_DIR/many/$PREFIX$removedexport"
    ls $file
    ($?) || error "$file wasn't removed yet, but a lookup failed!"

    # copy export config from server
    scp root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE $BUILD_TEST_DIR/$prevexportconfig
    cp $BUILD_TEST_DIR/$EXPORTFILE $BUILD_TEST_DIR/$newexportconfig
    
    # generate new exports and append to server's export config
    $BUILD_TEST_DIR/ganesha_configs/createmanyexports.pl \
        $FS_DIR \
        $PREFIX \
        $GANVERSION \
        $DIRNUMBER \
        $FSAL $removedexport >> $BUILD_TEST_DIR/$newexportconfig
    
    # copy export config to server and restart
    scp $BUILD_TEST_DIR/$newexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
    ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"    
    
    # First get the inode number from NFS
    file="$TEST_ROOT_DIR/many/$PREFIX$removedexport"
    ls $file
    (! $?) || error "$file was removed, but a lookup succeeded!"

    # restore previous config and restart
    scp $BUILD_TEST_DIR/$prevexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
    ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"    

}

### test_remove_readdir: 
SPEED[test_remove_readdir]=fast
TEST_TYPE[test_remove_readdir]=dev,admin,prod
NEEDS_ROOT[test_remove_readdir]=yes
PRE_TEST[test_remove_readdir]=setup_pseudofs
function test_remove_readdir
{
    prevexportconfig=$EXPORTFILE.fromremovereaddir
    newexportconfig=$EXPORTFILE.pseudofstest
    removedexport=20

    # first have client cache result of pseudofs node
    file="$TEST_ROOT_DIR/many"
    ls $file | grep "$PREFIX$removedexport"
    ($?) || error "$file wasn't removed yet, but readdir isn't showing it!"

    # copy export config from server
    scp root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE $BUILD_TEST_DIR/$prevexportconfig
    cp $BUILD_TEST_DIR/$EXPORTFILE $BUILD_TEST_DIR/$newexportconfig
    
    # generate new exports and append to server's export config
    $BUILD_TEST_DIR/ganesha_configs/createmanyexports.pl \
        $FS_DIR \
        $PREFIX \
        $GANVERSION \
        $DIRNUMBER \
        $FSAL $removedexport >> $BUILD_TEST_DIR/$newexportconfig
    
    # copy export config to server and restart
    scp $BUILD_TEST_DIR/$newexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
    ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"    
    
    # First get the inode number from NFS
    file="$TEST_ROOT_DIR/many"
    ls $file | grep "$PREFIX$removedexport"
    (! $?) || error "$file was removed, but a readdir still lists it!"

    # restore previous config and restart
    scp $BUILD_TEST_DIR/$prevexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
    ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"    

}

### test_remove_stat: 
SPEED[test_remove_stat]=fast
TEST_TYPE[test_remove_stat]=dev,admin,prod
NEEDS_ROOT[test_remove_stat]=yes
PRE_TEST[test_remove_stat]=setup_pseudofs
function test_remove_stat
{
    prevexportconfig=$EXPORTFILE.fromremovereaddir
    newexportconfig=$EXPORTFILE.pseudofstest
    removedexport=20

    # first get a list of inode numbers for files
    file="$TEST_ROOT_DIR/many/$PREFIX$removedexport"
    ls $file
    ($?) || error "$file wasn't removed yet, but a lookup failed!"

    # copy export config from server
    scp root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE $BUILD_TEST_DIR/$prevexportconfig
    cp $BUILD_TEST_DIR/$EXPORTFILE $BUILD_TEST_DIR/$newexportconfig
    
    # generate new exports and append to server's export config
    $BUILD_TEST_DIR/ganesha_configs/createmanyexports.pl \
        $FS_DIR \
        $PREFIX \
        $GANVERSION \
        $DIRNUMBER \
        $FSAL $removedexport >> $BUILD_TEST_DIR/$newexportconfig
    
    # copy export config to server and restart
    scp $BUILD_TEST_DIR/$newexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
    ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"    
    
    # Now check if the inode numbers are consistent after removal of pseudo nodes
    file="$TEST_ROOT_DIR/many"
    ls $file | grep "$PREFIX$removedexport"
    (! $?) || error "$file was removed, but a readdir still lists it!"

    # restore previous config and restart
    scp $BUILD_TEST_DIR/$prevexportconfig root@$SERVERHOSTNAME:$EXPORTDIR/$EXPORTFILE
    ssh -tt root@$SERVER service nfs-ganesha-gpfs restart || error "Could not restart ganesha server over ssh"    

}

# syntax: ONLY=2,3 ./run_test.sh [-j] <test_dir>

######################## DEFINE TEST LIST HERE ####################

function run_pseudofs
{
  run_test setup_pseudofs      " PSEUDOFS: Preparing server for tests                             "

  run_test test_wr             " PSEUDOFS: Attempt write of pseudofs node                         "
  run_test test_rd             " PSEUDOFS: Attempt read of pseudofs node                          "
  run_test test_rm             " PSEUDOFS: Attempt removal of pseudofs node                       "
  run_test test_lookup         " PSEUDOFS: Lookup a single pseudfs node                           "
  run_test test_readdir        " PSEUDOFS: Execute readdirplus on large directory                 "
  run_test test_ls_junction    " PSEUDOFS: Cross a junction point and list directory              "
  run_test test_check_inode    " PSEUDOFS: Check if junction shows fs inode                       "
  run_test test_remove_lookup  " PSEUDOFS: Remove export from server, make sure lookup fails      "
#  run_test test_remove_readdir " PSEUDOFS: Remove export from server, make sure it's gone in dir  "
#  run_test test_remove_stat    " PSEUDOFS: Remove export from server, make sure inodes stay same  "
#  run_test test_add_lookup     " PSEUDOFS: Add an export from server, make sure it's gone         "
#  run_test test_add_stat       " PSEUDOFS: Add an export from server, make sure inodes stay same  "

  run_test cleanup_pseudofs    " PSEUDOFS: Cleaning up server environment                         "
}

