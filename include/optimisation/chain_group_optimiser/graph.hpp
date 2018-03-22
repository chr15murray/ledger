#ifndef OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#define OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#include"assert.hpp"
#include"crypto/fnv.hpp"
#include"byte_array/const_byte_array.hpp"
#include"memory/rectangular_array.hpp"
#include"commandline/vt100.hpp"
#include<unordered_map>
#include<map>
#include<memory>
namespace fetch {
namespace optimisers {

struct Block {
  std::vector< std::shared_ptr< Block > > previous;
  std::vector< std::shared_ptr< Block > > next;
  
  std::unordered_set< uint32_t > groups;
  uint64_t block = uint64_t(-1);
  double work = 0;
  double total_work = 0;  
  bool in_use = false;
};
  
  
class GroupGraph : public memory::RectangularArray< uint64_t > {
public:
  typedef crypto::CallableFNV hasher_type;
  typedef byte_array::ConstByteArray byte_array_type;

  enum {
    EMPTY = uint64_t(-1)    
  };
  
  GroupGraph(std::size_t const &blocks, std::size_t const &groups):
    memory::RectangularArray< uint64_t >(blocks, groups )
  {
    for(auto &a: *this) a = EMPTY;    
//    bricks_.resize(blocks);
    bricks_at_block_.resize(blocks);
    block_number_.resize(groups);
    chains_.resize(groups);
    
    for(auto &b: block_number_) b = 0;    
  }
    
  uint64_t AddBlock(double work, byte_array_type const &hash,
    std::vector< byte_array_type > const &previous_blocks,
    std::unordered_set< uint32_t > groups) {

    /*
    std::cout << "Adding : "<< hash << " ";
    for(auto &g: groups) std::cout << g << ", ";
    
    std::cout << previous_blocks.size() <<" ;; ";
      for(auto &p: previous_blocks) {
        std::cout << p << ", ";          
      }
      
      std::cout << std::endl;
    */  
    if(name_to_id_.find(hash) != name_to_id_.end() ) {
      TODO_FAIL("Hash already exist: ", hash);      
    }

    for(auto &h: previous_blocks) {
      if(name_to_id_.find( h ) == name_to_id_.end() ) {
        TODO_FAIL("previous not found");
      }      
    }

    
    name_to_id_[hash] = counter_;
    id_to_name_[counter_] = hash;
    uint64_t id = counter_;
    ++counter_;

    
    std::shared_ptr< Block > brick = std::make_shared< Block >( );
    brick->groups = groups;
    brick->block = id;    

    for(auto &h: previous_blocks) {
      auto id = name_to_id_[h];
      auto ptr = bricks_[id];
      
      brick->previous.push_back( ptr );
      ptr->next.push_back( brick ); 
    }
    
    
    bricks_.push_back( brick );    
    if(previous_blocks.size() == 0 ) {
      next_blocks_.insert( id );
      next_refs_[ id ] = 1; 
    }
    
    return id;
  }


  Block BlockFromGroups(std::unordered_set< uint64_t > const &groups) {
    Block ret;

    for(auto &g: groups) {
      auto &chain = chains_[g];
      if(chain.size() > 0) {
        ret.previous.push_back( chain.back() );        
      }
    }
    
    return ret;
  }
  
  void Shift() 
  {
    // TODO: shift
    ++block_offset_;    
  }


  std::vector< std::shared_ptr< Block > > & bricks(std::size_t const&i) 
  {
    return bricks_at_block_[i];
  }

  std::vector< std::shared_ptr< Block > > const & bricks(std::size_t const&i) const
  {
    return bricks_at_block_[i];
  }
  
  
  byte_array_type name_from_id(uint64_t const& i) const
  {
    return id_to_name_.find(i)->second;
  }

  uint64_t id_from_name(byte_array_type const& name) const
  {
    return name_to_id_.find(name)->second;
  }

  bool Activate(uint64_t block) 
  {
    if(used_blocks_.find(block) != used_blocks_.end()) {
      return false;      
    }
    uint64_t block_n = 0;
    auto &b = bricks_[block];

    /*
    std::cout << "Activating : "<< block << " / " << b->block << " " << name_from_id(block) << ": ";
    for(auto &g: b->groups) std::cout << g << ", ";
    
    std::cout << b->previous.size() <<" ;; ";
      for(auto &p: b->previous) {
        std::cout << name_from_id(p->block) << ", ";          
      }
      
      std::cout << std::endl;
    */    
    
    for(auto &g: b->groups) {
      block_n = std::max( block_n,  block_number_[g] );
    }
    if(block_n >= height() ) return false;
    
    std::unordered_map< uint64_t, int > prev_blocks;    

    for(auto &g: b->groups) {
      //      std::cout << "  - From group " << g << std::endl;
        
      auto &chain = chains_[g];
      if(chain.size() != 0) {
        if( prev_blocks.find( chain.back()->block ) == prev_blocks.end() ) {
          //          std::cout << "    > Setting " << chain.back()->block << " " << name_from_id(chain.back()->block) << std::endl;  
          prev_blocks[ chain.back()->block ] = 1;
        } else {
          //          std::cout << "    > Increasing" << std::endl;  
          ++prev_blocks[ chain.back()->block ];
        }
        
      }
    }

    bool ret = true;    
    for(auto &p: b->previous) {
      if(prev_blocks.find(p->block) == prev_blocks.end()) {
        /*
          std::cout << "  - Block not in previous: " << name_from_id(p->block) << " " << p->block << std::endl;
        for(auto &p: prev_blocks) {
          std::cout << p.first << ": " << p.second << std::endl;
            
        }
        */
        ret = false;
        break;        
      }      
      else
      {
        if(prev_blocks[ p->block ] == 0) {
          //          std::cout << "  - Something went wrong!! " << name_from_id(p->block) << std::endl;             
          ret = false;
          break;            
        }
        //        std::cout << "  - Decreasing" << std::endl;
          
        --prev_blocks[ p->block ];
      }
    }      

    
    for(auto &pb: prev_blocks) {
      if(pb.second !=0) {
        //        std::cout << "  - ?? " << pb.second << std::endl;             
        ret = false;
        break;        
      }     
    }

    if(ret) {
      for(auto&g: b->groups) {
        chains_[g].push_back(b);
        block_number_[g] = block_n + 1;

      }
      //      std::cout << " === Block number: " << block_n << " === " <<  std::endl;
      bricks_at_block_[block_n].push_back(b);
    }
    

    return ret;    
  }

  
  std::unordered_set< uint64_t > const &next_blocks() const {
    return next_blocks_;
  }
private:
  std::vector< std::vector< std::shared_ptr< Block > > > chains_;
  std::vector< uint64_t > block_number_;
  
  std::unordered_set< uint64_t > used_blocks_;
  

  
  std::unordered_set< uint64_t > next_blocks_;
  std::unordered_map< uint64_t, int > next_refs_;  
  std::vector< std::shared_ptr< Block > > bricks_;
  std::vector< std::vector< std::shared_ptr< Block > > > bricks_at_block_;  
  std::unordered_map<byte_array_type, uint64_t, hasher_type>  name_to_id_;
  std::unordered_map<uint64_t, byte_array_type>  id_to_name_;
  uint64_t counter_ = 0;
  uint64_t block_offset_ = 0;
  double total_work_ = 0;
  
};


std::ostream& operator<< (std::ostream& stream, GroupGraph const &graph )
{
  std::size_t lane_width = 3;
  std::size_t lane_width_half = lane_width >> 1;  
  std::size_t ww = graph.width() ;    
  std::size_t w = ww>> 1;
  std::size_t lane_size = lane_width * ww;
  auto DrawLane = [lane_width, ww, w, lane_size, lane_width_half, graph](std::vector< std::shared_ptr< Block > > const &bricks, std::size_t &n) {

    std::size_t start = ww;
    std::size_t end = 0;
    
    uint64_t block = uint64_t(-1);
        
    std::shared_ptr< Block > brick;
      
    if( (n < bricks.size()) )  {
      brick = bricks[n];     
      for(auto const &g: brick->groups) {
        if(g < start) start = g;
        if(g > end) end = g;          
      }
      block = brick->block;
    }
    
    bool has_block = start <= end;
    
    for(std::size_t i=0; i < ww; ++i)
    {
      bool embedded  = has_block && (start <= i)  && (i <= end);      
      bool left  = has_block && (start < i)  && (i <= end);
      bool right = has_block && (start <= i)  && (i < end);
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << ( (left) ? "-" : " " );

      if(embedded) {        
        std::cout << (( (brick->groups.find( i ) != brick->groups.end())  ) ? "*" :"-");
      } else {
        std::cout << "|";        
      }
      
      
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << ( (right) ? "-" : " " );                      
    }

    if(block != uint64_t(-1)) {
      std::cout << ": " << graph.name_from_id(block) << " >> ";
      for(auto &p : brick->previous) {
        std::cout << graph.name_from_id( p->block) << ", ";        
      }
      
    }
    ++n;

  };

  std::size_t total_transactions = 0;
  for(std::size_t i=0; i < graph.height() ; ++i)
  {
    auto const &bricks = graph.bricks(i);
    if(bricks.size() == 0) break;

    std::cout << " ";    
    for(std::size_t i=0; i < ww; ++i)
    {      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout <<  "=";

      std::cout << "=";
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << "=" ;    
    }
    
    
    std::cout << " ### Block " << i << ", " << bricks.size() << " transactions" << std::endl;
    total_transactions += bricks.size();
    assert(bricks.size() <= ww);
    std::size_t n = 0;
    
    for(std::size_t j=0; j < bricks.size() ; ++j)
    {
      std::cout << " ";
      DrawLane(bricks, n);        
      std::cout << std::endl;
    }

    
  }
  std::cout << "Total transactions = " << total_transactions << std::endl;
  return stream;  
}  

};
};

#endif