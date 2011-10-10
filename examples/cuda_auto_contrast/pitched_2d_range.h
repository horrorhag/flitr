#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/iterator/permutation_iterator.h>
#include <thrust/functional.h>

/// Based on the strided_range thrust example.
/// Given a MallocPitched 2D array, we want to iterate only over the valid elements.
/// The class assumes that the pitch size in bytes is divisable by the element size. Iow a row contains an integer number of elements.
template <typename InIterator>
class pitched_2d_range
{
  public:
    typedef typename thrust::iterator_difference<InIterator>::type difference_type;

    struct pitch_functor : public thrust::unary_function<difference_type, difference_type>
    {
        const difference_type _width;
        const difference_type _padded_width;
        const bool _no_padding;

        pitch_functor(difference_type width, difference_type padded_width)
            : _width(width), _padded_width(padded_width), _no_padding(width == padded_width) {}

        __host__ __device__
        difference_type operator()(const difference_type& i) const
        { 
            if (_no_padding) return i;

            // we need to convert from the unpadded index to the padded
            difference_type row = i / _width;
            // save a division versus i % _width
            difference_type col = i - (row * _width);
            return row * _padded_width + col;
        }
    };

    typedef typename thrust::counting_iterator<difference_type> CountingIterator;
    typedef typename thrust::transform_iterator<pitch_functor, CountingIterator> TransformIterator;
    typedef typename thrust::permutation_iterator<InIterator,TransformIterator> PermutationIterator;

    // type of the pitched_2d_range iterator
    typedef PermutationIterator iterator;

    // construct our iterator for the range [first,last)
    pitched_2d_range(InIterator first, InIterator last, difference_type width, difference_type padded_width)
        : _first(first), _last(last), _width(width), _padded_width(padded_width) {}
   
    iterator begin(void) const
    {
        return PermutationIterator(_first, TransformIterator(CountingIterator(0), pitch_functor(_width, _padded_width)));
    }

    iterator end(void) const
    {
        /// convert from padded to unpadded size
        return begin() + ((_last - _first)/_padded_width)*_width;
    }
    
  protected:
    difference_type _width;
    difference_type _padded_width;
    InIterator _first;
    InIterator _last;
};
