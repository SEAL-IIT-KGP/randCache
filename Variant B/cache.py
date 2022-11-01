#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue May 18 17:53:27 2021

@author: anirban
"""
from bin_addr import BinaryAddress
from word_addr import WordAddress
from reference import ReferenceCacheStatus
from reference import ReferenceEvictionStatus
import random

class Cache(dict):
    partition = None
    cal_index = None
    count_ref_index = 0
    def __init__(self, data_store = None, tag_store = None, num_data_blocks = None, num_sets_per_skew = None, num_index_bits = None, num_partitions = None, num_tag_blocks_per_skew = None, num_addr_bits = None, num_offset_bits = None, num_total_ways = None):
       
        self.recently_used_addrs = []
        if tag_store is not None:
            self.update(tag_store)
        else:
            for j in range(num_partitions):
                for i in range(num_tag_blocks_per_skew):                
                    index = BinaryAddress(word_addr = WordAddress(i), num_addr_bits = num_addr_bits)[(num_addr_bits - num_offset_bits - num_index_bits):(num_addr_bits - num_offset_bits)]
                    self[str(j)+str(index)] = []
                    for k in range(num_total_ways):
                        if (random.randint(0, 1)  == 0) :
                            self[str(j)+str(index)].append({'valid': 0}) 
                        else:
                            self[str(j)+str(index)].append({'valid': 1})
                    
                    
    def mark_ref_as_last_seen(self, ref):
        addr_id = (ref.index, ref.tag)
        if addr_id in self.recently_used_addrs:
            self.recently_used_addrs.remove(addr_id)
        self.recently_used_addrs.append(addr_id)
        
        
    def load_balancing(self, index_tuple, num_partitions):
#        print(index_tuple)
        block1 = self[str(0)+str(index_tuple[0])]
        block2 = self[str(1)+str(index_tuple[1])]
        block1_valid_count = 0
        block2_valid_count = 0
#        print(block1)
#        print(block2)
        for block in block1:
            if block['valid']:
                block1_valid_count += 1
        for block in block2:
            if block['valid']:
                block2_valid_count += 1
        if block1_valid_count < block2_valid_count:
 #           print("valid eviction")
            return (0, block1_valid_count)
        elif block1_valid_count > block2_valid_count:
#            print("valid eviction")
            return (1, block2_valid_count)
        else:
            
            randint = random.randint(0,1)
            if (randint == 0):
                return (0, block1_valid_count)
            else:
                return (1, block2_valid_count)
            
        
    def is_hit(self, addr_index, addr_tag, num_partitions):
        global partition
        global cal_index
        num_index_bits = int(len(addr_index[0]))
        blocks = []
        if addr_index[0] is None:
            blocks = self[str(0).zfill(num_index_bits)]
        else:
            for i in range(num_partitions):
                actual_index = str(i)+str(addr_index[i])
                empty_set = True 
                if (actual_index) in self:
                    blocks = self[actual_index]
#                    print(blocks)
                    for block in blocks:
                        if 'tag' in block.keys():
                            empty_set = False
                            break
                    if empty_set == True:
                        continue
                    else:
                        for block in blocks:
                            if ('tag' in block.keys() and block['tag'] == addr_tag):
                                partition = i
                                cal_index = addr_index[i]
                                return True
                else:
                    return False    
        return False                    
                    
                
    def replace_block(self, blocks, replacement_policy, num_tags_per_set, skew, valid_count, num_partition, addr_index, new_entry, count_ref_index):
#        print("replacing block")
        if (replacement_policy == 'rand'):
            if (valid_count < num_tags_per_set):
                repl_block_index = -1
                for (index, block) in enumerate(blocks):
                    if block['valid'] == 0:
#                        print('replacing: '+ str(index))
                        repl_block_index = index
                        break
                blocks[repl_block_index] = new_entry
                return
            else:
                print("valid eviction")
                repl_block_index = random.randint(0, num_tags_per_set - 1)
                for (index, block) in enumerate(blocks):
                    if (index == repl_block_index):
                        blocks[index] = new_entry
                        return
#
#        if (replacement_policy == 'lru'):
#            recently_used_addrs = self.recently_used_addrs
#            for recent_index, recent_tag in recently_used_addrs:
#                for i, block in enumerate(blocks):
#                    if (recent_index == addr_index and block['tag'] == recent_tag):
#                        blocks[i] = new_entry
#                        return
                
    def set_block(self, replacement_policy, num_tags_per_set, num_partition, addr_index, new_entry, count_ref_index):
        global partition
        global cal_index
        num_index_bits = int(len(addr_index[0]))
#        print(addr_index, num_index_bits)
        if addr_index[0] is None:
            blocks = self[str(0).zfill(num_index_bits)]
        else:
#            block1 = self[str(0)+str(addr_index[0])]
#            block2 = self[str(1)+str(addr_index[1])]
#            print(block1, block2)
            skew, count_valid = self.load_balancing(index_tuple = addr_index, num_partitions = num_partition)            

            blocks = self[str(skew)+str(addr_index[skew])]
 #           print(blocks)
#        print("len block, num blocks per set: "+str(len(blocks))+" "+str(num_blocks_per_set))
            self.replace_block(blocks, replacement_policy, num_tags_per_set, skew, count_valid, num_partition, addr_index[skew], new_entry, count_ref_index)

#        print(self)
        partition = skew
        cal_index = addr_index[skew]
            
            
    def read_refs(self, num_total_tags_per_set, num_partitions, replacement_policy, num_words_per_block, refs):
        count_ref_index = 0
        for ref in refs:
#            self.mark_ref_as_last_seen(ref)
#            print("----------------------------------- starting a ref")
#            print(ref.index)
            if self.is_hit(ref.index, ref.tag, num_partitions):
                ref.cache_status = ReferenceCacheStatus.hit
                ref.partition = partition
                ref.index = cal_index
                ref.valid = 1
                ref.eviction_type = ReferenceEvictionStatus.valid

            else:
#                print("inside the miss part")
                ref.cache_status = ReferenceCacheStatus.miss
                ref.eviction_type = ReferenceEvictionStatus.invalid
                self.set_block(
                        replacement_policy = replacement_policy,
                        num_tags_per_set = num_total_tags_per_set,
                        num_partition = num_partitions,
                        addr_index = ref.index,
                        new_entry = ref.get_cache_entry(num_words_per_block),
                        count_ref_index = count_ref_index
                        )
                ref.partition = partition
                ref.index = cal_index
                ref.valid = 1
            count_ref_index += 1