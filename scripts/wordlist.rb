#!/usr/bin/env ruby
require "minitest/autorun"


# how many characters match
def matching_prefix(a, b)
  a
    .chars
    .zip(b.chars)
    .map{|a, b| a == b}
    .take_while{|x| x}
    .count
end

class WordNode
  attr_accessor :children, :word, :leaf

  def initialize(word: nil, leaf: false)
    @leaf = leaf
    @word = word
    @children = []
  end

  def add(word)
    longest_matching_prefix, index = @children
      .each_with_index
      .map{|child, index| [matching_prefix(child.word, word), index]}
      .max_by{|child, index| index}

    if (longest_matching_prefix || 0) == 0
      # no matching child, or no children: create child
      @children << WordNode.new(word: word, leaf: true)
      @children.sort_by!{|x| x.word}
    elsif @children[index].word == word
      # exact match: make sure it's set to leaf
      @children[index].leaf = true
    elsif longest_matching_prefix < @children[index].word.chars.count
      # need to split node to insert this
      new = WordNode.new(word: word.chars[0...longest_matching_prefix].join, leaf: false)
      new.children << @children[index]
      @children[index] = new
      new.children[0].word = new.children[0].word.chars[longest_matching_prefix..-1].join
      add(word)
    else
      # word is longer than child: recurse
      @children[index].add(word.chars[longest_matching_prefix..-1].join)
    end
  end
end

class WordNodeTest < Minitest::Test
  def test_that_adding_word_creates_word
    root = WordNode.new()

    root.add('anon')
    root.add('nuna')
    root.add('loli')

    assert_equal root.children.count, 3
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[1].word, 'loli'
    assert_equal root.children[2].word, 'nuna'
    assert_equal root.children[0].leaf, true
    assert_equal root.children[1].leaf, true
    assert_equal root.children[2].leaf, true
  end

  def test_that_adding_existing_word_marks_leaf
    root = WordNode.new()

    root.add('anon')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[0].leaf, true

    root.add('anon')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[0].leaf, true

    root.children[0].leaf = false

    root.add('anon')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[0].leaf, true
  end

  def test_that_adding_existing_longer_word
    root = WordNode.new()

    root.add('anon')
    root.add('anonymous')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[0].leaf, true
    assert_equal root.children[0].children.count, 1
    assert_equal root.children[0].children[0].leaf, true
    assert_equal root.children[0].children[0].word, 'ymous'

    root.add('anonguy')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[0].leaf, true
    assert_equal root.children[0].children.count, 2
    assert_equal root.children[0].children[0].leaf, true
    assert_equal root.children[0].children[0].word, 'guy'
    assert_equal root.children[0].children[1].leaf, true
    assert_equal root.children[0].children[1].word, 'ymous'
  end

  def test_can_insert_partially_existing
    root = WordNode.new()

    root.add('anonymous')
    root.add('anon')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'anon'
    assert_equal root.children[0].leaf, true
    assert_equal root.children[0].children.count, 1
    assert_equal root.children[0].children[0].leaf, true
    assert_equal root.children[0].children[0].word, 'ymous'
    assert_equal root.children[0].children[0].children.count, 0

    root.add('anna')

    assert_equal root.children.count, 1
    assert_equal root.children[0].word, 'an'
    assert_equal root.children[0].leaf, false
    assert_equal root.children[0].children.count, 2
    assert_equal root.children[0].children[0].leaf, true
    assert_equal root.children[0].children[0].word, 'na'
    assert_equal root.children[0].children[0].children.count, 0
    assert_equal root.children[0].children[1].leaf, true
    assert_equal root.children[0].children[1].word, 'on'
    assert_equal root.children[0].children[1].children.count, 1
  end
end
