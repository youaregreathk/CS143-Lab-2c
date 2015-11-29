#include "BTreeNode.h"
#include <math.h>
#include <iostream>
#include <cstring>

using namespace std;
/******************************************************************************
 Lab 2b
 
 ******************************************************************************/
/*
struct BTLeafNode::Entry{
  RecordId rid;
  int key;
};
*/


//Leaf node Constructor
BTLeafNode::BTLeafNode()
{
    memset(buffer,0,PageFile::PAGE_SIZE);
    //std::fill(buffer, buffer + PageFile::PAGE_SIZE, 0);
    
    MaximunKeyNumber = (PageFile::PAGE_SIZE - sizeof(PageId)) / sizeof(entry);
    
}


/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{

    return pf.read(pid, buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
         return pf.write(pid, buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
    
    
    int* ptr = 252+(int*)&buffer[0];
    int tmp;
    tmp = *ptr;
    return tmp;
 
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
    int eid = 0;
    //We return full if the the getkeyCount is larger or equal than the maxkeyNum
    if(getKeyCount() >= MaximunKeyNumber){
        return RC_NODE_FULL;
    }
    
    
    locate(key, eid);
    int* tmp = (int*) buffer;
    int i = getKeyCount();
    while(i >= eid){
        *(tmp + i*3 + 2) = *(tmp + i*3 + 2 -3);*(tmp + i*3 + 1) = *(tmp + i*3 +1 -3);*(tmp + i*3) = *(tmp + i*3 -3);
        i--;
    }
    *(tmp +eid*3 +2) = key;
    *(tmp +eid*3 +1) = rid.sid;
    *(tmp +eid*3) = rid.pid;
    
    
    int* total = (int*)&buffer[0]+252;
    *total = getKeyCount()+1;
    
    return 0;

}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{
  //if(getKeyCount() <  maxKeyNum || sibling.getKeyCount()!=0)
  // return RC_NODE_FULL;
    int index = getKeyCount()/2;
    int x=index;
    while(x < getKeyCount()){
       int tkey;
        RecordId trid;
        readEntry(x,tkey,trid);
        sibling.insert(tkey,trid);
        x++;
    }
    
    int* p_end = 252 + (int*)buffer;
    *p_end = index;
    
    int eid = 0;
    locate(key, eid);
    if (eid >= index) 
        sibling.insert(key, rid);
    else
        insert(key, rid);

    RecordId irid;
    sibling.readEntry(0,siblingKey,irid);
    return 0;

}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
                   behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{
    int* ptr = (int*)buffer;
    eid = 0;
    int x=0;
    while(x < getKeyCount()){
    if (*(ptr+2+3*x) >= searchKey)
            break;
        x++;
    }
    eid = x;
    
    return 0;
   
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid){
    if( eid < 0)
        return RC_INVALID_CURSOR;
    
    if (eid >= getKeyCount() )
        return RC_INVALID_CURSOR;
    
    int* tmp_ptr = (int*)buffer;
    rid.sid = *( 3*eid+tmp_ptr  + 1);rid.pid = *(3*eid +tmp_ptr );key = *(3*eid+tmp_ptr +2);
    return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node
 */
PageId BTLeafNode::getNextNodePtr(){
    int* tmp_ptr = (int*) ((PageFile::PAGE_SIZE-sizeof(PageId))+buffer);int pid = *tmp_ptr;
    return pid;
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
    int* tmp_ptr = (int*) (buffer+(PageFile::PAGE_SIZE-sizeof(PageId)));*tmp_ptr = pid;
    return 0;
}


/*
 *
 *
 * NON-LEAF-NODE
 *
 *
 */

//Constructor to allocate memory, copied from the LeafNode Constructor
BTNonLeafNode::BTNonLeafNode()
{
    MaximunKeyNumber = (PageFile::PAGE_SIZE - sizeof(PageId))/(sizeof(int) + sizeof(PageId)) -2;
    memset(buffer,0,PageFile::PAGE_SIZE);
}



/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{
    return pf.read(pid, buffer);
}

/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
    return pf.write(pid, buffer);;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */

//The key count is store at the position buffer[0] + 252*sizeof(int)
int BTNonLeafNode::getKeyCount()
{
    int *tmp_ptr = (int*)&buffer[0] + 252;
    int result = *tmp_ptr;
    return result;
    
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
    
    int* tmp_ptr = (int*)buffer + 1;
    int eid = 0;
    int k = 0;
    int target = 0;
    
    if (getKeyCount() >= MaximunKeyNumber){
        return RC_NODE_FULL;
    }
    
    if (tmp_ptr!=NULL) {
        while( key > target)
        {
            target = *(k+tmp_ptr);
            k += 2;
            eid++;
            if(target==0)
                break;
        }
    }
    
    int j = getKeyCount();
    while(j >= eid){
       *(tmp_ptr+2*j) = *(tmp_ptr+2*j-2);*(tmp_ptr+2*j+1) = *(tmp_ptr+2*j-1);
       j--;
    }
    if (eid != 0)
        eid--;
    
    *(tmp_ptr + eid * 2) = key;*(tmp_ptr+ eid * 2 + 1) = pid;  //We insert the record here
    
    int* pCount = 252+(int*)&buffer[0];
    *pCount = getKeyCount() + 1;
    
    return 0;
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey){
    int half = getKeyCount()/2;
    int* tmp_ptr = (int*)buffer + 1;
    
    int i=half;
    while(i < getKeyCount()){
        int tkey;
        
        PageId tpid;
        tkey = *(tmp_ptr + i*2);tpid = *(tmp_ptr +1 +i*2);
        
        if(sibling.getKeyCount() > 0)
            sibling.insert(tkey,tpid);
        else{
            PageId ipid;
            ipid = *(tmp_ptr + i*2 -1);
            sibling.initializeRoot(ipid,tkey,tpid);
        }
        i++;
    }
    
    if (key >= *(tmp_ptr + half*2))
        sibling.insert(key,pid);
    else
        insert(key,pid);
    
    midKey = getKeyCount()-1;
    key = *(tmp_ptr + midKey*2 );
    midKey = key;
    
    int* pCount = 252 + (int*)buffer;
    *pCount = half;
    return 0;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
    int* tmp_ptr = (int*) buffer + 1;
    int i= 0;
    while (i < getKeyCount()) {
        if ( *tmp_ptr > searchKey){
            pid = *(tmp_ptr-1);
            break;
        }else if(*tmp_ptr > searchKey){
            pid = *(tmp_ptr-1);
            break;
        }
        tmp_ptr = tmp_ptr + 2;
        i ++;
    }
    pid = *(tmp_ptr-1);
    return 0;
    
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
    int* tmp_ptr = (int*) buffer;
    *tmp_ptr = pid1;
    insert(key, pid2);
    int* p_end = (int*)&buffer[0]+252;    //Here we change the key count
    *p_end = 1;
    return 0;
    
}
