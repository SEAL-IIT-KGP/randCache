#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue May 18 17:53:27 2021

@author: anirban
"""
from bin_addr import BinaryAddress
from word_addr import WordAddress
from reference import ReferenceCacheStatus
import random

class Cache(dict):
    partition = None
    cal_index = None
    def __init__(self, cache=None, num_sets=None, num_index_bits=None, num_partitions = None, ways_per_partition = None):
       
        self.recently_used_addrs = []
        if cache is not None:
            self.update(cache)
        else:
            for j in range(num_partitions):
                for i in range(num_sets):                
                    index = BinaryAddress(word_addr = WordAddress(i), num_addr_bits = num_index_bits)
                    self[str(j)+str(index)] = []            
            
    def mark_ref_as_last_seen(self, ref):
        addr_id = (ref.index, ref.tag)
        if addr_id in self.recently_used_addrs:
            self.recently_used_addrs.remove(addr_id)
        self.recently_used_addrs.append(addr_id)
        
        
    def is_hit(self, addr_partition, addr_index, addr_tag, num_partitions):
        global partition
        global cal_index
        num_index_bits = int(len(addr_index) / num_partitions)
        blocks = []
        if addr_index is None:
            blocks = self[str(0).zfill(num_index_bits)]
        else:
            num_index_bits = int(len(addr_index) / num_partitions)
            for i in range(num_partitions):
                start = len(addr_index) - ((i + 1) * num_index_bits)
                end = len(addr_index) - (i * num_index_bits)
                actual_index = addr_index[start:end]
                if (str(i)+str(actual_index)) in self:
                    if self[(str(i)+str(actual_index))] == []:
                        continue
                    else:
                        blocks = self[str(i)+str(actual_index)]
                        for block in blocks:
                            if (block['tag'] == addr_tag):
                                partition = i
                                cal_index = actual_index
                                return True
                else:
                    return False
    
        return False
                
    def replace_block(self, blocks, replacement_policy, num_blocks_per_set, addr_partition, num_partition, addr_index, new_entry):
        if (replacement_policy == 'rand'):
            repl_block_index = random.randint(0, (num_blocks_per_set // num_partition) - 1)
            for (i, block) in enumerate(blocks):
                if (i == repl_block_index):
                    blocks[i] = new_entry
                    return
        if (replacement_policy == 'lru'):
            recently_used_addrs = self.recently_used_addrs
            for recent_index, recent_tag in recently_used_addrs:
                for i, block in enumerate(blocks):
                    if (recent_index == addr_index and block['tag'] == recent_tag):
                        blocks[i] = new_entry
                        return
                
    def set_block(self, replacement_policy, num_blocks_per_set, addr_partition, num_partition, addr_index, new_entry):
        num_index_bits = int(len(addr_index) / num_partition)
        if addr_index is None:
            blocks = self[str(0).zfill(num_index_bits)]
        else:
            start = len(addr_index) - ((addr_partition + 1) * num_index_bits)
            end = len(addr_index) - (addr_partition  * num_index_bits) 
            addr_index = addr_index[start:end]
            blocks = self[str(addr_partition)+ (str(addr_index).zfill(num_index_bits))]
        if (len(blocks) == (num_blocks_per_set // num_partition)):
            self.replace_block(blocks, replacement_policy, num_blocks_per_set, addr_partition, num_partition, addr_index, new_entry)
        else:
            blocks.append(new_entry)
            
            
    def read_refs(self, num_blocks_per_set, num_words_per_block, num_partitions, replacement_policy, refs):
        for ref in refs:
            self.mark_ref_as_last_seen(ref)
            if self.is_hit(ref.partition, ref.index, ref.tag, num_partitions):
                ref.cache_status = ReferenceCacheStatus.hit
                ref.partition = partition
                ref.index = cal_index
                    
            else:
                ref.cache_status = ReferenceCacheStatus.miss
                self.set_block(
                        replacement_policy = replacement_policy,
                        num_blocks_per_set = num_blocks_per_set,
                        addr_partition = ref.partition,
                        num_partition = num_partitions,
                        addr_index = ref.index,
                        new_entry = ref.get_cache_entry(num_words_per_block)
                        )
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        