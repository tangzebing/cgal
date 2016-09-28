// Copyright (c) 2016  GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Simon Giraudot

#ifndef CGAL_POINT_SET_3_H
#define CGAL_POINT_SET_3_H

#include <stack>

#include <CGAL/Surface_mesh/Properties.h>

namespace CGAL {



/*!

  \ingroup PkgPointSet3

  \brief A collection of points with dynamically associated
  properties.

  This class is a range of indices that provides the user with a
  flexible way to store and access a point set:

  - it can store an arbitrary number of additional attributes such as
    normal vectors, colors, indices, etc.;

  - all functions of the package \ref PkgPointSetProcessing are
    provided with an overload that take a `Point_set_3` object as an
    argument.

  \tparam Point Point type.
  \tparam Vector Normal vector type.

  \cgalModels `Range`
 */

template <typename Point,
          typename Vector = typename Kernel_traits<Point>::Kernel::Vector_3>
class Point_set_3
{
public:

  /// \cond SKIP_IN_MANUAL
  typedef Point Point_type;
  typedef Vector Vector_type;
  typedef Point_set_3<Point, Vector> Point_set;

  class Index;

  typedef typename Properties::Property_container<Index> Base;

  template <class Type>
  struct Property_map
    : public Properties::Property_map<Index, Type>
  {
  };
  typedef Property_map<Index> Index_map;

  template <typename Property>
  class Push_property_map;
  /// \endcond
  
  /*!
    \brief This represents a point with associated properties.
    \cgalModels `Index`
    \cgalModels `LessThanComparable`
    \cgalModels `Hashable`
  */
  class Index
  {
    /// \cond SKIP_IN_MANUAL
    friend class Point_set_3;
    friend class Properties::Property_container<Index>;
    template <class> friend class Properties::Property_array;
    template <class> friend class Property_map;
    template <class> friend class Push_property_map;
    friend class std::vector<Index>;
    std::size_t value;
    
    // Only Point_set_3 and other friend classes are allowed to
    // instantiate an Index with a specific value
    Index (const std::size_t& value) : value (value) { }
    /// \endcond
  public:
    Index (const Index& index) : value (index) { }
    Index () : value ((std::size_t)(-1)) { }
    Index operator= (const Index& index) { value = index.value; return *this; }
    operator std::size_t() const { return value; }
    bool operator== (const Index& index) const { return value == index.value; }
    bool operator!= (const Index& index) const { return value != index.value; }
    bool operator<  (const Index& index) const { return value < index.value; }
    Index& operator++ () { ++ value; return *this; }
    Index& operator-- () { -- value; return *this; }
    Index operator++ (int) { Index tmp(*this); ++ value; return tmp; }
    Index operator-- (int) { Index tmp(*this); -- value; return tmp; }
  };
  

#ifdef DOXYGEN_RUNNING
  typedef unspecified_type iterator; ///< Iterator type of the point set \cgalModels RandomAccessIterator
  typedef unspecified_type const_iterator; ///< Constant iterator type of the point set \cgalModels RandomAccessIterator
#else
  typedef typename Index_map::iterator iterator; ///< Iterator type of the point set
  typedef typename Index_map::const_iterator const_iterator; ///< Constant iterator type of the point set
#endif

  typedef Property_map<Point> Point_map; ///< Property map of points
  typedef Property_map<Vector> Vector_map; ///< Property map of vectors

  /// \cond SKIP_IN_MANUAL
  template <class Type>
  class Property_range
  {
  public:
    typedef CGAL::Property_map_to_unary_function<Property_map<Type> > Unary_function;
    typedef boost::transform_iterator<Unary_function,
                                      Point_set::const_iterator> const_iterator;
  private:
    const_iterator m_begin;
    const_iterator m_end;
    std::size_t m_size;
    
  public:
    Property_range (const Property_map<Type>& pmap, Point_set::const_iterator begin, Point_set::const_iterator end,
           std::size_t size)
    {
      m_begin = boost::make_transform_iterator (begin, Unary_function(pmap));
      m_end = boost::make_transform_iterator (end, Unary_function(pmap));
      m_size = size;
    }
    
    const_iterator begin() const { return m_begin; }
    const_iterator end() const { return m_end; }
    std::size_t size() const { return m_size; }
    bool empty() const { return (m_size == 0); }
  };
  /// \endcond

  typedef Property_range<Point> Point_range; ///< Constant range of points
  typedef Property_range<Vector> Vector_range; ///< Constant range of vectors

protected:

  /// \cond SKIP_IN_MANUAL
  Base m_base;
  Index_map m_indices;
  Point_map m_points;
  Vector_map m_normals;
  std::size_t m_nb_removed;
  /// \endcond
  
public:

  /// \name Constructor
  /// @{


  /*!
    \brief Create an empty point set with no additional property.
   */
  Point_set_3 (bool with_normal_map = false) : m_base()
  {
    clear();
    if (with_normal_map)
      add_normal_map();
  }

  /// \cond SKIP_IN_MANUAL
  Point_set_3 (const Point_set_3& ps)
  {
    *this = ps;
  }
  /// \endcond


  /// @}

  /// \name Accessors and Modifiers
  /// @{


  /*!
    \brief Assignment operator.

    All properties with their content are copied.
   */
  Point_set_3& operator= (const Point_set_3& ps)
  {
    m_base = ps.m_base;
    m_indices = this->property_map<Index> ("index").first;
    m_points = this->property_map<Point> ("point").first;
    m_normals = this->property_map<Vector> ("normal").first;
    m_nb_removed = ps.m_nb_removed;
    return *this;
  }

  /*!
    \brief Merge `other` in the point set.

    Shifts the indices of points of `other` by `number_of_points() +
    other.number_of_points()`.

    Copies entries of all property maps which have the same name in
    the point set and `other`.  Property maps which are only in
    `other` are ignored.

    \note Garbage is collected in both point sets when calling this function.
   */
  bool join (Point_set_3& other)
  {
    collect_garbage();
    other.collect_garbage();
    resize (number_of_points() + other.number_of_points());
    m_base.transfer (other.m_base);
    
    // Reset indices
    for (std::size_t i = 0; i < this->m_base.size(); ++ i)
      this->m_indices[i] = i;

    return true;
  }
  
  /*!
    \brief Clears the point set attributes and content.

    After calling this function, the object is the same as a newly
    constructed object. The additional attributes (such as normal vectors)
    are removed too and must be added again if they are still needed.
   */
  void clear()
  {
    m_base.clear();
    boost::tie (m_indices, boost::tuples::ignore) = this->add_property_map<Index>("index", (std::size_t)(-1));
    boost::tie (m_points, boost::tuples::ignore) = this->add_property_map<Point>("point", Point (0., 0., 0.));
    m_nb_removed = 0;
  }

  /*!
    \brief Memory management: reserve size to make the following insertions quicker.

    \param s Expected number of element to be inserted next.

    \note This method does not change the content of the point set and
    is only used for optimization.
   */
  void reserve (std::size_t s) { m_base.reserve (s); }
  
  /*!
    \brief Change size of the point set.

    \param s Target size of the point set.

    \note Garbage may be collected if needed.
   */
  void resize (std::size_t s)
  {
    if (s < number_of_points() + number_of_removed_points())
      {
        collect_garbage();
        if (s < number_of_points())
          m_base.resize (s);
        else
          {
            std::size_t prev_s = number_of_points();
            m_base.resize (s);
            for (std::size_t i = prev_s; i < s; ++ i)
              m_indices[i] = i;
          }
      }
    else
      {
        std::size_t prev_s = number_of_points() + number_of_removed_points();
        m_base.resize (s);
        for (std::size_t i = prev_s; i < s; ++ i)
          m_indices[i] = i;
      }
  }

  /*!
    \brief Insert new element with default property values.
   */
  iterator insert ()
  {
    if (m_nb_removed == 0)
      {
        m_base.push_back();
        m_indices[size()-1] = size()-1;
        return m_indices.end() - 1;
      }
    else
      {
        -- m_nb_removed;
        return m_indices.end() - m_nb_removed - 1;
      }
  }

  /*!
    \brief Convenience function to add a point.

    \param p Point to insert

    \note Properties of the added point are initialized to their
    default value.
   */
  iterator insert (const Point& p)
  {
    iterator out = insert();
    m_points[size()-1] = p;
    return out;
  }

  /*!
    \brief Convenience function to add a point with a normal vector.

    \param p Point to insert
    \param n Associated normal vector

    \note Properties of the added point other than its normal vector
    are initialized to their default value.

    \note A normal property must have been added to the point set
    before using this method.
   */
  iterator insert (const Point& p, const Vector& n)
  {
    iterator out = insert (p);
    assert (has_normals());
    m_normals[size()-1] = n;
    return out;
  }

  /*!
    \brief Return the begin iterator.
  */
  iterator begin() { return m_indices.begin(); }
  /*!
    \brief Return the past-the-end iterator.
    \note The returned value is the same as `garbage_begin()`.
  */
  iterator end() { return m_indices.end() - m_nb_removed; }
  /*!
    \brief Return the begin constant iterator.
  */
  const_iterator begin() const { return m_indices.begin(); }
  /*!
    \brief Return the past-the-end constant iterator.
    \note The returned value is the same as `garbage_begin()`.
  */
  const_iterator end() const { return m_indices.end() - m_nb_removed; }
  /*!
    \brief Returns `true` if the number of non-removed element is 0.

    \note This does not count the removed elements.

    \note The method `empty()` is also available (see `Range`) and
    does the same thing.
  */
  bool is_empty() const { return (m_base.size() == m_nb_removed); }
  /// \cond SKIP_IN_MANUAL
  bool empty() const { return is_empty(); }
  /// \endcond
  /*!
    \brief Returns the number of elements (not counting removed one).

    \note See `number_of_removed_points()` for getting the number of removed elements.

    \note The method `size()` is also available (see `Range`) and
    does the same thing.
  */
  std::size_t number_of_points () const { return m_base.size() - m_nb_removed; }
  /// \cond SKIP_IN_MANUAL
  std::size_t size () const { return number_of_points(); }
  /// \endcond

  /*!
    \brief Get a constant reference to the wanted indexed point.

    \param index Index of the wanted point.
  */
  /*!
    \brief Get a reference to the wanted indexed point.

    \param index Index of the wanted point.
  */
  Point& point (const Index& index) { return m_points[index]; }
  /*!
    \brief Get a constant reference to the wanted indexed point.

    \param index Index of the wanted point.
  */
  const Point& point (const Index& index) const { return m_points[index]; }
  /*!
    \brief Get a reference to the wanted indexed normal.

    \param index Index of the wanted normal.

    \note The normal property must have been added to the point set
    before calling this method (see `add_normal_map()`).
  */
  Vector& normal (const Index& index) { return m_normals[index]; }
  /*!
    \brief Get a constant reference to the wanted indexed normal.

    \param index Index of the wanted normal.

    \note The normal property must have been added to the point set
    before calling this method (see `add_normal_map()`).
  */
  const Vector& normal (const Index& index) const { return m_normals[index]; }

  /// @}

  /// \name Garbage Management
  /// @{

  /*!
    \brief Mark all elements between `first` and `last` as removed.

    \note The elements are just marked as removed and are not erased
    from the memory. `collect_garbage()` should be called if the
    memory needs to be freed.
  */
  void remove (iterator first, iterator last)
  {
    if (std::distance (last, end()) < 0)
      last = end();

    if (last == end())
      m_nb_removed += static_cast<std::size_t>(std::distance (first, end()));
    if (std::distance (first, end()) > 0)
      {
        iterator source = first;
        iterator dest = end() - 1;
        m_nb_removed += static_cast<std::size_t>(std::distance (first, last));
        while (source != last // All elements have been moved
               && dest != last - 1) // All elements are at the end of the container
          {
            std::cerr << "Swapping " << *source << " and " << *dest << std::endl;
            std::swap (*(source ++), *(dest --));
          }
      }
  }

  /// \cond SKIP_IN_MANUAL
  void remove_from (iterator first)
  {
    remove (first, end());
  }
  /// \endcond
  
  /*!
    \brief Mark element as removed.

    \note The element is just marked as removed and is not erased from
    the memory. `collect_garbage()` should be called if the memory
    needs to be freed.
  */
  void remove (iterator it)
  {
    std::iter_swap (it, (end() - 1));
    ++ m_nb_removed;
  }

  /*!
    \brief Returns `true` if the element is marked as removed.

    \note When iterating between `begin()` and `end()`, no element
    marked as removed can be found.
  */
  bool is_removed (const_iterator it) const
  {
    return (std::distance (it, garbage_begin()) < 0);
  }

  /*!
    \brief Constant iterator to the first removed element (equal to `garbage_end()` if
    no elements are marked as removed.
  */
  const_iterator garbage_begin () const { return m_indices.end() - m_nb_removed; }
  /*!
    \brief Past-the-end constant iterator of the removed elements.
  */
  const_iterator garbage_end () const { return m_indices.end(); }
  /*!
    \brief Number of removed points.
  */
  std::size_t number_of_removed_points () const { return m_nb_removed; }
  /// \cond SKIP_IN_MANUAL
  std::size_t garbage_size () const { return number_of_removed_points(); }
  /// \endcond
  /*!
    \brief Returns `true` if there are still removed elements in memory.
  */
  bool has_garbage () const { return (m_nb_removed != 0); }  

  /*!
    \brief Erase from memory the elements marked as removed.
  */
  void collect_garbage ()
  {
    // Indices indicate where to get the properties
    std::vector<std::size_t> indices (m_base.size());
    for (std::size_t i = 0; i < m_base.size(); ++ i)
      indices[m_indices[i]] = i;

    // Indices now indicate where to put the properties
    for (std::size_t i = 0; i < m_base.size(); ++ i)
      m_indices[i] = indices[i];

    // for (std::size_t i = 0; i < 10; ++ i)
    //   std::cerr << m_indices[i] << " ";
    // std::cerr << std::endl;

    // Sorting based on the indices reorders the point set correctly
    quick_sort_on_indices ((std::ptrdiff_t)0, (std::ptrdiff_t)(m_base.size() - 1));

    // for (std::size_t i = 0; i < 10; ++ i)
    //   std::cerr << m_indices[i] << " ";
    // std::cerr << std::endl;

    m_base.resize (size ());
    m_base.shrink_to_fit ();
    m_nb_removed = 0;
  }

  /// @}


  /*! \name Property Handling

    A property `Property_map<Type>` allows to associate properties of
    type `Type` to an indexed point. Properties can be added, and
    looked up with a string, and they can be removed at runtime.
  */

  /// @{

#ifdef DOXYGEN_RUNNING
  /// Model of `LvaluePropertyMap` with `Index` as a key type and `Type`
  /// as value type.
  template <class Type>
  using Property_map = unspecified_type;
  
  /// Model of `ConstRange` that handles constant ranges for property
  /// maps with value type `Type`.
  template <class Type>
  using Property_range = unspecified_type;

#endif

  
  /*!
    \brief Test if property `name` of type `T` already exists.

    \tparam T type of the property.

    \param name Name of the property.
  */
  template <typename T>
  bool has_property_map (const std::string& name) const
  {
    std::pair<typename Properties::template Property_map<Index, T>, bool>
      pm = m_base.template get<T> (name);
    return pm.second;
  }
  
  /*!
    \brief Adds a new property `name` of type `T` with given default value.

    \tparam T type of the property.

    \param name Name of the property.

    \param t Value taken by the property on already created elements.

    \return Returns a pair containing the property map and a Boolean
    that is `true` if the property was added and `false` if it already
    exists (and was therefore not added but only returned).
  */
  template <class T>
  std::pair<Property_map<T>, bool>
  add_property_map (const std::string& name, const T t=T())
  {
    Properties::Property_map<Index,T> pm;
    bool added = false;
    boost::tie (pm, added) = m_base.template add<T> (name, t);
    return std::make_pair (reinterpret_cast<Property_map<T>&>(pm), added);
  }
  
  /*!
    \brief Gets the property `name` of type `T`.

    \tparam T type of the property.

    \param name Name of the property.

    \return Returns a pair containing: the wanted property map and a
    Boolean set to `true` or an empty property map and a Boolean set
    to `false` (if the property was not found).
  */
  template <class T> 
  std::pair<Property_map<T>,bool>
  property_map (const std::string& name) const
  {
    Properties::Property_map<Index,T> pm;
    bool okay = false;
    boost::tie (pm, okay) = m_base.template get<T>(name);
    return std::make_pair (reinterpret_cast<Property_map<T>&>(pm), okay);
  }

  /*!
    \brief Get a constant range for property `pmap`.
  */
  template <class T>
  Property_range<T> range (const Property_map<T>& pmap) const
  {
    return Property_range<T> (pmap, begin(), end(), number_of_points());
  }
  
  /*!
    \brief Removes the wanted property.

    \tparam T type of the property.

    \param prop The property.

    \return Returns `true` if the property was removed and `false` if
    the property was not found.
  */
  template <class T> 
  bool remove_property_map (Property_map<T>& prop)
  {
    return m_base.remove (reinterpret_cast<Properties::Property_map<Index,T>&>(prop));
  }

  /*!
    \brief Convenience method that tests if the point set has normals.

    This method tests if a property of type `Vector` and named
    `normal` exists.
  */
  bool has_normal_map() const
  {
    std::pair<Vector_map, bool> pm = this->property_map<Vector> ("normal");
    return pm.second;
  }
  /*!
    \brief Convenience method that adds a normal property.

    This method adds a property of type `Vector` and named
    `normal`.

    \return `true` if the property was added, `false` if it already
    existed.
  */
  bool add_normal_map (const Vector& default_value = Vector(0., 0., 0.))
  {
    bool out = false;
    boost::tie (m_normals, out) = this->add_property_map<Vector> ("normal", default_value);
    return out;
  }
  /*!
    \brief Get the property map of the normal attribute.

    \note The normal property must have been added to the point set
    before calling this method (see `add_normal_map()`).
  */
  Vector_map normal_map ()
  {
    return m_normals;
  }
  /*!
    \brief Get the property map of the normal attribute (constant version).

    \note The normal property must have been added to the point set
    before calling this method (see `add_normal_map()`).
  */
  const Vector_map normal_map () const
  {
    return m_normals;
  }
  /*!
    \brief Get a constant range of normals.
  */
  Vector_range normals () const
  {
    return this->range<Vector> (m_normals);
  }
  /*!
    \brief Convenience method that removes the normal property.

    \return Returns `true` if the property was removed and `false` if
    the property was not found.
  */
  bool remove_normal_map()
  {
    return m_base.remove (m_normals);
  }
  /*!
    \brief Get the property map of the point attribute.
  */
  Point_map point_map()
  {
    return m_points;
  }

  /*!
    \brief Get the property map of the point attribute (constant version).
  */
  const Point_map point_map() const
  {
    return m_points;
  }
  /*!
    \brief Get a constant range of points.
  */
  Point_range points () const
  {
    return this->range<Point> (m_points);
  }

  /*!
    \brief List properties with their types in a `std::string` object.
  */
  std::string properties() const
  {
    std::ostringstream oss;
    oss << "CGAL::Point_set_3<" << boost::core::demangle(typeid(Point).name())
        << "> with " << size() << " point(s) ("
        << number_of_removed_points() << " removed point(s) waiting to be deleted)" << std::endl;
    std::vector<std::string> prop = m_base.properties();
    for (std::size_t i = 0; i < prop.size(); ++ i)
      oss << " * \"" << prop[i] << "\" property of type "
          << boost::core::demangle(m_base.get_type(prop[i]).name()) << std::endl;

    return oss.str();
  }
  /// @}

  
  /*!
  
    \ingroup PkgPointSet3

    \brief Reads the point set from an input stream that can be either:

    - XYZ
    - OFF
    - PLY

    The format is detected from the stream. If the stream contains
    normal vectors, the normal map is added to the point set. For PLY
    input, all point properties found in the header are added.
    \relates Point_set_3
  */
  template <typename P>
  friend std::istream& operator>>(std::istream& is, Point_set_3<P>& ps)
  {
    // Check format identifier on first line
    // std::string line;
    // if (!getline(stream, line))
    //   return is;
    // stream.seekg(0);
    // if (line == "OFF" || line == "NOFF")
    //   CGAL::read_off_point_set (is, ps);
    // else if (line == "ply")
    //   CGAL::read_ply_point_set (is, ps);
    // else
    //   CGAL::read_xyz_point_set (is, ps);
    
    return is;
  }
  /*!
  
    \ingroup PkgPointSet3

    \brief Inserts the point set in an output stream in Ascii PLY
    format. All properties are inserted in their instantiation order.

    \relates Point_set_3
  */
  template <typename P>
  friend std::ostream& operator<<(std::ostream& os, const Point_set_3<P>& ps)
  {
    os << "ply" << std::endl
       << "format ascii 1.0" << std::endl
       << "comment Generated by the CGAL library" << std::endl
       << "element vertex " << ps.number_of_points() << std::endl;
  
    std::vector<std::string> prop = ps.m_base.properties();  
    for (std::size_t i = 0; i < prop.size(); ++ i)
      {
        if (prop[i] == "index")
          continue;
        else if (prop[i] == "point")
          os << "property double x" << std::endl
             << "property double y" << std::endl
             << "property double z" << std::endl;
        else if (prop[i] == "normal")
          os << "property double nx" << std::endl
             << "property double ny" << std::endl
             << "property double nz" << std::endl;
        else if (ps.m_base.template get<boost::int8_t>(prop[i]).second)
          os << "property char " << prop[i] << std::endl;
        else if (ps.m_base.template get<boost::uint8_t>(prop[i]).second)
          os << "property uchar " << prop[i] << std::endl;
        else if (ps.m_base.template get<boost::int16_t>(prop[i]).second)
          os << "property short " << prop[i] << std::endl;
        else if (ps.m_base.template get<boost::uint16_t>(prop[i]).second)
          os << "property ushort " << prop[i] << std::endl;
        else if (ps.m_base.template get<boost::int32_t>(prop[i]).second)
          os << "property int " << prop[i] << std::endl;
        else if (ps.m_base.template get<boost::uint32_t>(prop[i]).second)
          os << "property uint " << prop[i] << std::endl;
        else if (ps.m_base.template get<float>(prop[i]).second)
          os << "property float " << prop[i] << std::endl;
        else if (ps.m_base.template get<double>(prop[i]).second)
          os << "property double " << prop[i] << std::endl;
        else
          os << "property " << boost::core::demangle(ps.m_base.get_type(prop[i]).name())
             << " " << prop[i] << std::endl;
      }
    
    os << "end_header" << std::endl;  

    for (const_iterator it = ps.begin(); it != ps.end(); ++ it)
      os << ps.m_base.to_str(*it);
    return os;
  }


  /*!
    \name Push Property Maps and Inserters (Advanced)

    \cgalAdvancedBegin
    The following method are specifically designed to make
    `CGAL::Point_set_3` usable with \cgal input/output functions.
    \cgalAdvancedEnd
  */

  /// @{

  
#ifdef DOXYGEN_RUNNING
  /// \cgalAdvancedBegin  
  /// Model of `OutputIterator` used to insert elements by defining
  /// the value of the property `Property`.
  /// \cgalAdvancedEnd  
  template <class Property>
  using Property_back_inserter = unspecified_type;

  /// \cgalAdvancedBegin  
  /// Model of `WritablePropertyMap` based on `Property` and that
  /// pushes a new item to the point set if needed.
  /// \cgalAdvancedEnd
  template <class Property>
  using Push_property_map = unspecified_type;
#endif
  
  /// \cond SKIP_IN_MANUAL  
  template <typename Property>
  class Property_back_inserter {

  public:
    typedef std::output_iterator_tag iterator_category;
    typedef typename Property::value_type value_type;
    typedef std::ptrdiff_t           difference_type;
    typedef void                     pointer;
    typedef void                     reference;

  private:

    Point_set* ps;
    Property* prop;
    Index ind;
  
  public:
    
    Property_back_inserter(Point_set* ps, Property* prop, Index ind=Index())
      : ps(ps), prop (prop), ind(ind) {}
    Property_back_inserter& operator++() { return *this; }
    Property_back_inserter& operator++(int) { return *this; }
    Property_back_inserter& operator*() { return *this; }
    Property_back_inserter& operator= (const value_type& p)
    {
      if(ps->size() <= ind)
        ps->insert();
      put(*prop, ind, p);
      ++ ind;
      return *this;
    }

  };

  template <typename Property>
  class Push_property_map
  {

  public:
    typedef Index key_type;
    typedef typename Property::value_type value_type;
    typedef value_type& reference;
    typedef boost::lvalue_property_map_tag category;
    
    Point_set* ps;
    Property* prop;
    mutable Index ind;

    Push_property_map(Point_set* ps = NULL,
                      Property* prop = NULL,
                      Index ind=Index())
      : ps(ps), prop(prop), ind(ind) {}

    friend void put(const Push_property_map& pm, Index& i, const reference t)
    {
      if(pm.ps->size() <= (pm.ind))
        pm.ps->insert();
      put(*(pm.prop), pm.ind, t);
      i = pm.ind;
      ++pm.ind;
    }

    friend const reference get (const Push_property_map& pm, const Index& i)
    {
      return ((*(pm.prop))[i]);
    }

  };
  /// \endcond      

  /// \cgalAdvancedBegin
  /// Back inserter on indices
  /// \cgalAdvancedEnd
  typedef Property_back_inserter<Index_map> Index_back_inserter; 
  /// \cgalAdvancedBegin
  /// Back inserter on points
  /// \cgalAdvancedEnd
  typedef Property_back_inserter<Point_map> Point_back_inserter;
  /// \cgalAdvancedBegin
  /// Property map for pushing new points
  /// \cgalAdvancedEnd
  typedef Push_property_map<Point_map> Point_push_map;
  /// \cgalAdvancedBegin
  /// Property map for pushing new vectors
  /// \cgalAdvancedEnd
  typedef Push_property_map<Vector_map> Vector_push_map;

  /*!
    \cgalAdvancedBegin
    \cgalAdvancedFunction
    \brief Gets the push property map of the given property.

    \tparam T type of the property.

    \param prop The property map.

    \return Returns a pair containing: the wanted property map and a
    Boolean set to `true` or an empty property map and a Boolean set
    to `false` (if the property was not found).
    \cgalAdvancedEnd
  */
  template <class T>
  Push_property_map<Property_map<T> >
  push_property_map (Property_map<T>& prop)
  {
    return Push_property_map<Property_map<T> > (this, &prop, size());
  }
  /*!
    \cgalAdvancedBegin
    \cgalAdvancedFunction
    \brief Get the push property map of the point attribute.
    \cgalAdvancedEnd
  */
  Point_push_map point_push_map ()
  {
    return Point_push_map (this, &m_points, size());
  }
  /*!
    \cgalAdvancedBegin
    \cgalAdvancedFunction
    \brief Get the push property map of the normal attribute.

    \note The normal property must have been added to the point set
    before calling this method (see `add_normal_map()`).
    \cgalAdvancedEnd
  */
  Vector_push_map normal_push_map ()
  {
    return Vector_push_map (this, &m_normals, size());
  }
  /*!
    \cgalAdvancedBegin
    \cgalAdvancedFunction
    \brief Get the back inserter on the index attribute.
    \cgalAdvancedEnd
  */
  Index_back_inserter index_back_inserter ()
  {
    return Index_back_inserter (this, &m_indices, size());
  }
  /*!
    \cgalAdvancedBegin
    \cgalAdvancedFunction
    \brief Get the back inserter on the point attribute.
    \cgalAdvancedEnd
  */
  Point_back_inserter point_back_inserter ()
  {
    return Point_back_inserter (this, &m_points, size());
  }

  /// @}

  
private:
  /// \cond SKIP_IN_MANUAL
  void quick_sort_on_indices (std::ptrdiff_t begin, std::ptrdiff_t end)
  {
    std::stack<std::pair<std::ptrdiff_t, std::ptrdiff_t> >
      todo;
    todo.push (std::make_pair (begin, end));
    
    while (!(todo.empty()))
      {
        std::pair<std::ptrdiff_t, std::ptrdiff_t>
          current = todo.top();
        todo.pop();
        
        if (current.first < current.second)
          {
            std::ptrdiff_t p = current.first + (rand() % (current.second - current.first));
            p = quick_sort_partition (current.first, current.second, p);
            todo.push (std::make_pair (current.first, p-1));
            todo.push (std::make_pair (p+1, current.second));
          }
      }
  }

  std::ptrdiff_t quick_sort_partition (std::ptrdiff_t begin, std::ptrdiff_t end, std::ptrdiff_t p)
  {
    m_base.swap (p, end);
    std::ptrdiff_t j = begin;
    for (std::ptrdiff_t i = begin; i < end; ++ i)
      if (m_indices[i] <= m_indices[end])
        {
          m_base.swap (i, j);
          j ++;
        }
    m_base.swap (end, j);
    return j;
  }
  /// \endcond

  
}; // end of class Point_set_3




/*!

  \ingroup PkgPointSet3

  \brief Inserts `other` into `ps`.

  \relates Point_set_3
  
   Shifts the indices of points of `other` by `ps.number_of_points() +
   other.number_of_points()`.

   Copies entries of all property maps which have the same name in `ps` and `other`. 
   Property maps which are only in `other` are ignored.

   \note Garbage is collected in both point sets when calling this function.

*/
template <typename Point, typename Vector>
Point_set_3<Point, Vector>& operator+=(Point_set_3<Point, Vector>& ps,
                                       Point_set_3<Point, Vector>& other)
{
  ps.join(other);
  return ps;
}





} // namespace CGAL


#endif // CGAL_POINT_SET_3_H
