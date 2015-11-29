/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include <iostream>
#include <cstdio>

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
	// need to be fixed
	//int bufSize = PageFile::PAGE_SIZE;
	//int pidSize = sizeof(PageId);
	//int intSize = sizeof(int);
	int totalLeafSize = PageFile::PAGE_SIZE - sizeof(PageId) - sizeof(int);
	int eachLeafSize = 2*sizeof(int) + sizeof(PageId);
	int totalNonLeafSize = PageFile::PAGE_SIZE - sizeof(PageId);
	int eachNonLeafSize = sizeof(int) + sizeof(PageId);
	leafKeyNum = totalLeafSize/eachLeafSize;
	nonleafKeyNum = totalNonLeafSize/eachNonLeafSize - 2;
	//leafKeyNum = (PageFile::PAGE_SIZE - sizeof(PageId) - sizeof(int)) / (sizeof(int)+sizeof(PageId)+sizeof(int));
    //nonleafKeyNum = (PageFile::PAGE_SIZE - sizeof(PageId))/(sizeof(int) + sizeof(PageId)) -2;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
	if (pf.open(indexname, mode))	return pf.open(indexname, mode);
	if (pf.endPid())
	{
		if (pf.read(0, buffer))	return pf.read(0, buffer);
	}
	else
	{
        treeHeight = 0; rootPid = -1; close();
		return open(indexname,mode);
	}

    PageId* tmp = (PageId*) buffer; treeHeight = *(tmp + 1); rootPid = *tmp;
   
    return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
	PageId* tmp = (PageId*) buffer; *(tmp + 1) = treeHeight; *tmp = rootPid;
    if (pf.write(0, buffer))	return pf.write(0, buffer);
    return pf.close();
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
	if (!treeHeight)
	{
        BTLeafNode leafNode; BTNonLeafNode nonLeafNode;
        leafNode.insert(key, rid); treeHeight++; rootPid = pf.endPid();
        if (!rootPid)	rootPid++;
        int rc = leafNode.write(rootPid, pf);
        if (leafNode.write(rootPid, pf))	return leafNode.write(rootPid, pf);
	}
	else
	{
		int ofKey; PageId ofPid;
        if(RecInsert(key,rid,rootPid,1,ofKey,ofPid) == 1)	
        	return createNewRoot(ofKey, rootPid, ofPid);
	}
    return 0;
}

/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */


RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
	BTLeafNode leafNode;
	BTNonLeafNode nonLeafNode;
	int res, eid = 0; PageId pid = rootPid;
	for (int i = 0; i < treeHeight - 1; i++)
	{
		if (nonLeafNode.read(pid, pf))	return RC_FILE_READ_FAILED;
		if (nonLeafNode.locateChildPtr(searchKey, pid))	return RC_INVALID_CURSOR;
	}

	res = leafNode.read(pid, pf);
	if(res)	return RC_FILE_READ_FAILED;
	res = leafNode.locate(searchKey, eid);
    cursor.eid = eid; cursor.pid = pid;

    return res;
}


/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
	if (cursor.pid)
	{
		BTLeafNode leafNode;
        if (leafNode.read(cursor.pid, pf))	return  RC_FILE_READ_FAILED;
        if (cursor.eid < leafNode.getKeyCount()-1)	cursor.eid++;
        else
        {
         	cursor.eid = 0; cursor.pid = leafNode.getNextNodePtr();     
        }
        return leafNode.readEntry(cursor.eid, key, rid);
	}
	else
    	return RC_END_OF_TREE;;
}


RC BTreeIndex::createNewRoot(int key, PageId curRootPid, PageId newRootPid)
{
    BTNonLeafNode Root;	int res = Root.initializeRoot(curRootPid, key, newRootPid);
    rootPid = pf.endPid();
    Root.write(rootPid, pf); treeHeight++;
    return res;
}



RC BTreeIndex::RecInsert(int& key, const RecordId& rid, PageId pid, int lev, int& ofKey, PageId& ofPage){
    
    int res;
    if(lev != treeHeight)
    {

    	PageId childPid; BTNonLeafNode nonLeafNode; nonLeafNode.read(pid,pf);        
 
        nonLeafNode.locateChildPtr(key,childPid);
        
    
        if(RecInsert(key,rid,childPid,lev+1,ofKey,ofPage) == 1){
            if(nonLeafNode.getKeyCount() >= nonleafKeyNum)
            {
            	BTNonLeafNode sib;   
                nonLeafNode.insertAndSplit(ofKey,ofPage,sib,ofKey);
                ofPage = pf.endPid();
                sib.write(ofPage,pf);
                nonLeafNode.write(pid,pf);
                return 1;                
            }
            else
            {
                res = nonLeafNode.insert(ofKey,ofPage); res = nonLeafNode.write(pid,pf);
                ofKey = 0;
            }
        } 
    }
    else
    {
        BTLeafNode leafNode;
        leafNode.read(pid,pf);
        if(leafNode.getKeyCount() >= leafKeyNum)
        {
        	BTLeafNode sib;
            leafNode.insertAndSplit(key,rid,sib,ofKey);
            ofPage = pf.endPid();
            sib.setNextNodePtr(leafNode.getNextNodePtr());
            leafNode.setNextNodePtr(ofPage);
            leafNode.write(pid,pf);
            sib.write(ofPage,pf);
            return 1;
        }
        else
        {
			res = leafNode.insert(key,rid); res = leafNode.write(pid,pf);
        }
        
    }
    return res;
}

