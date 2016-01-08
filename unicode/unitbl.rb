#!/usr/bin/env ruby

require 'set'

UNICODE_DIR = 'data'

# Read a file line by line, discarding comments beginning with # and skipping
# empty and comment-only lines, and pass the lines to a block
def read_unicode_file(file)
    File.open(file, 'r') do |fp|
        fp.each_line do |line|
            sharp = line.index('#')
            line = line[0...sharp] unless sharp.nil?
            line.strip!
            yield line unless line.empty?
        end
    end
end

# As read_unicode_file, but split each line at any semicolons within it
def read_unicode_file_with_fields(file, &block)
    read_unicode_file(file) do |line|
        rec = line.split(';', -1).map {|x| x.strip}
        block.call(rec)
    end
end

# As read_unicode_file_with_fields, but the first field may be a range
def read_unicode_file_with_ranges(file, &block)
    read_unicode_file_with_fields(file) do |rec|
        bounds = rec[0].split('..')
        if bounds.size == 2 then
            (bounds[0].hex..bounds[1].hex).each do |cp|
                block.call(cp, rec)
            end
        else
            block.call(bounds[0].hex, rec)
        end
    end
end

# As read_unicode_file_with_fields, but adapted to UnicodeData.txt, which has
# the names ending with First> and Last> to indicate ranges with the same
# properties; for such ranges, pass each code point separately
def read_unicode_data(file, &block)
    first = nil
    read_unicode_file_with_fields(file) do |rec|
        if (rec[1][-6..-1] == 'First>') then
            first = rec[0].hex
        elsif (rec[1][-5..-1] == 'Last>') then
            (first..rec[0].hex).each do |cp|
                rec[0] = '%04X' % cp
                block.call rec
            end
        else
            block.call rec
        end
    end
end

# Write table as ranges of code points mapping to a common 8-bit string
def write_string_ranges(file, udata, field, default_value = nil)
    last = nil
    file.puts "static const string8_range_entry #{field}_table[] ="
    file.puts '{'
    udata.keys.sort.each do |cp|
        value = udata[cp][field]
        next if (value.nil? or value == default_value)
        if last.nil? or last[1]+1 != cp or last[2] != value then
            file.puts '    { 0x%04X, 0x%04X, "%s" },' % last unless last.nil?
            last = [ cp, cp, value ]
        else
            last[1] = cp
        end
    end
    file.puts '    { 0x%04X, 0x%04X, "%s" },' % last unless last.nil?
    file.puts '};'
    file.puts
end

# Write table of ranges of code points having a given Boolean property
def write_boolean_ranges(file, property, field)
    file.puts "static const boolean_entry #{field}_table[] ="
    file.puts '{'
    rec = nil
    0x110000.times do |cp|
        next unless property.member?(cp)
        if rec.nil? then
            rec = [cp, cp]
        elsif cp == rec[1] + 1  then
            rec[1] = cp
        else
            file.puts '    { 0x%04X, 0x%04X },' % rec
            rec = [cp, cp]
        end
    end
    file.puts '    { 0x%04X, 0x%04X },' % rec unless rec.nil?
    file.puts '};'
    file.puts
end

# Write table of mappings from a code point to one or more code points
def write_mapping_table(file, udata, field)
    chars = udata.keys.sort

    # Table of one-to-one mappings
    file.puts "static const char_entry #{field}1_table[] ="
    file.puts '{'
    chars.each do |cp|
        value = udata[cp][field]
        next if (value.nil? or value.size != 1)
        file.puts '    { 0x%04X, { 0x%04X, 0 } },' % [cp, value[0]]
    end
    file.puts '};'
    file.puts

    # Strings giving one-to-many mappings
    chars.each do |cp|
        value = udata[cp][field]
        next if (value.nil? or value.size < 2)
        m = (value.map {|x| "0x%04X" % x}).join(', ')
        file.puts 'static const utf32_t %s%04X[] = { %s, 0 };' % [field, cp, m]
    end
    file.puts

    # Table of one-to-many mappings
    file.puts "static const string_entry #{field}_table[] ="
    file.puts '{'
    chars.each do |cp|
        value = udata[cp][field]
        next if (value.nil? or value.size < 2)
        file.puts '    { 0x%04X, %s%04X },' % [cp, field, cp]
    end
    file.puts '};'
    file.puts
end

# Read UnicodeData.txt
unicode_data = {}
read_unicode_data("#{UNICODE_DIR}/UnicodeData.txt") do |rec|
    cp = rec[0].hex
    data = {}
    # rec[1] is the name
    data['gen_category'] = rec[2]
    data['comb_class'] = rec[3].to_i
    # rec[4] is the bidi class
    # Convert rec[5] to a decomposition type and mapping
    if (rec[5] != '') then
        # The decomposition has fields separated by spaces
        decomp = rec[5].split(' ')
        # The first field is <some_word> if we have a compatibility
        # decomposition; in that case, set dctype.  If the first field is
        # a hex integer, we have a canonical decomposition; leave dctype
        # as nil.
        if (decomp[0][0] == '<') then
            data['dctype'] = decomp.shift
        end
        # Decomposition as an array of integers
        data['decomp'] = decomp.map {|x| x.hex}
    end
    data['decimal_value'] = rec[6].to_i unless rec[6].empty?
    # rec[7] is the digit value
    # rec[8] is the numeric value
    # rec[9] is 'Y' if the character is mirrored, else 'N'
    # rec[10] is the Unicode 1.0 name
    # rec[11] is the ISO 10646 comment (no longer used since Unicode 5.2.0)
    # Simple case mappings; set as arrays of one integer, so that when
    # SpecialCasing.txt is read, these can also be arrays and the data
    # structures will be consistent.
    data['uppercase'] = [ rec[12].hex ] unless rec[12].empty?
    data['lowercase'] = [ rec[13].hex ] unless rec[13].empty?
    data['titlecase'] = [ rec[14].hex ] unless rec[14].empty?

    unicode_data[cp] = data
end

# Read CompositionExclusions.txt
compexclude = Set.new
read_unicode_file("#{UNICODE_DIR}/CompositionExclusions.txt") do |line|
    compexclude.add(line.hex)
end

# Read SpecialCasing.txt
read_unicode_file_with_fields("#{UNICODE_DIR}/SpecialCasing.txt") do |rec|
    next if (rec.size >= 5 and not rec[4].empty?)
    cp = rec[0].hex
    data = unicode_data[cp]
    # Lowercase mappings
    unless rec[1].empty?
        m = rec[1].split(' ').map {|x| x.hex}
        data['lowercase'] = m if (m.size > 1 or m[0] != cp)
    end
    # Titlecase mappings
    unless rec[2].empty?
        m = rec[2].split(' ').map {|x| x.hex}
        data['titlecase'] = m if (m.size > 1 or m[0] != cp)
    end
    # Uppercase mappings
    unless rec[3].empty?
        m = rec[3].split(' ').map {|x| x.hex}
        data['uppercase'] = m if (m.size > 1 or m[0] != cp)
    end
end

# Read CaseFolding.txt
read_unicode_file_with_fields("#{UNICODE_DIR}/CaseFolding.txt") do |rec|
    # Use default full case folding
    next unless (rec[1] == 'C' or rec[1] == 'F')
    cp = rec[0].hex
    unicode_data[cp]['casefold'] = rec[2].split(' ').map {|x| x.hex}
end

# Read EastAsianWidth.txt
read_unicode_file_with_ranges("#{UNICODE_DIR}/EastAsianWidth.txt") do |cp, rec|
    unicode_data[cp] ||= {}
    unicode_data[cp]['ea_width'] = rec[1]
end

# Read PropList.txt
properties = {}
read_unicode_file_with_ranges("#{UNICODE_DIR}/PropList.txt") do |cp, rec|
    prop = rec[1]
    properties[prop] ||= Set.new
    properties[prop].add(cp)
end

# This will be useful for several tables
chars = unicode_data.keys.sort

# Open the output file and generate a header
outfile = File.open('../include/unitbl.h', 'w')
outfile.puts '/* Generated from the Unicode data files by unitbl.rb.  Edits will be lost. */'
outfile.puts ''

# Decomposition table
write_mapping_table(outfile, unicode_data, 'decomp');

# The table of decomposition types
write_string_ranges(outfile, unicode_data, 'dctype')

# Composition table
composition = []
chars.each do |cp|
    data = unicode_data[cp]
    decomp = data['decomp']
    next if decomp.nil?
    next if decomp.size != 2
    next unless data['dctype'].nil?
    next if compexclude.member?(cp)
    next if data['comb_class'] != 0
    next if unicode_data[decomp[0]]['comb_class'] != 0
    composition.push [ decomp[0], decomp[1], cp ]
end
composition.sort! do |a, b|
    comp = a[0] <=> b[0]
    comp = a[1] <=> b[1] if comp == 0
    comp
end

outfile.puts 'static const comp_entry comp_table[] ='
outfile.puts '{'
composition.each do |c|
    outfile.puts "    { 0x%04X, 0x%04X, 0x%04X }," % c
end
outfile.puts '};'
outfile.puts ''

# General category
write_string_ranges(outfile, unicode_data, 'gen_category')

# Combining class
outfile.puts 'static const int_range_entry comb_class_table[] ='
outfile.puts '{'
last = nil
chars.each do |cp|
    data = unicode_data[cp]
    cclass = data['comb_class']
    next if (cclass.nil? or cclass == 0)
    if last.nil? or last[1]+1 != cp or last[2] != cclass then
        outfile.puts '    { 0x%04X, 0x%04X, %3d },' % last unless last.nil?
        last = [cp, cp, cclass]
    else
        last[1] = cp
    end
end
outfile.puts '    { 0x%04X, 0x%04X, %3d },' % last unless last.nil?
outfile.puts '};'
outfile.puts

# Decimal digit values
outfile.puts 'static const int_range_entry decimal_value_table[] ='
outfile.puts '{'
last = nil
chars.each do |cp|
    data = unicode_data[cp]
    value = data['decimal_value']
    next if value.nil?
    if last.nil? \
    or last[1]+1 != cp \
    or last[2]+last[1]-last[0]+1 != value then
        outfile.puts '    { 0x%04X, 0x%04X, %d },' % last unless last.nil?
        last = [cp, cp, value]
    else
        last[1] = cp
    end
end
outfile.puts '    { 0x%04X, 0x%04X, %d },' % last unless last.nil?
outfile.puts '};'
outfile.puts

# Lowercase mapping table
write_mapping_table(outfile, unicode_data, 'lowercase')

# Titlecase mapping table
write_mapping_table(outfile, unicode_data, 'titlecase')

# Uppercase mapping table
write_mapping_table(outfile, unicode_data, 'uppercase')

# Case folding table
write_mapping_table(outfile, unicode_data, 'casefold')

# East Asian width table
write_string_ranges(outfile, unicode_data, 'ea_width', 'N')

# Whitespace table
write_boolean_ranges(outfile, properties['White_Space'], 'whitespace')
