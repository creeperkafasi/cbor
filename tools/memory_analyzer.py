#!/usr/bin/env python3
"""
CBOR Library Memory Analysis Tool

This script provides comprehensive memory analysis for the CBOR library,
including stack analysis, memory mapping, and embedded safety guarantees.
"""

import os
import re
import json
import subprocess
import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple
import argparse

@dataclass
class FunctionInfo:
    name: str
    address: int
    size: int
    section: str
    stack_usage: Optional[int] = None
    calls: List[str] = field(default_factory=list)
    called_by: List[str] = field(default_factory=list)

@dataclass
class MemorySection:
    name: str
    address: int
    size: int
    usage_type: str  # 'code', 'data', 'bss', 'stack', 'heap'

@dataclass
class MemoryAnalysis:
    functions: Dict[str, FunctionInfo] = field(default_factory=dict)
    sections: Dict[str, MemorySection] = field(default_factory=dict)
    total_flash: int = 0
    total_ram: int = 0
    used_flash: int = 0
    used_ram: int = 0
    stack_size: int = 0
    max_call_depth: int = 0
    worst_case_stack: int = 0

class CBORMemoryAnalyzer:
    def __init__(self, map_file_path: str):
        self.map_file = Path(map_file_path)
        self.analysis = MemoryAnalysis()
        self.cbor_functions = set()
        
    def parse_map_file(self):
        """Parse the ARM linker map file for memory information."""
        print(f"📊 Parsing map file: {self.map_file}")
        
        with open(self.map_file, 'r') as f:
            content = f.read()
        
        # Parse memory configuration
        self._parse_memory_configuration(content)
        
        # Parse sections
        self._parse_memory_sections(content)
        
        # Parse function symbols
        self._parse_function_symbols(content)
        
        # Calculate memory usage
        self._calculate_memory_usage()
        
    def _parse_memory_configuration(self, content: str):
        """Extract memory configuration from map file."""
        memory_config_match = re.search(
            r'Memory Configuration\s*\n\s*Name\s+Origin\s+Length\s+Attributes\s*\n(.*?)\n\n',
            content, re.DOTALL
        )
        
        if memory_config_match:
            lines = memory_config_match.group(1).strip().split('\n')
            for line in lines:
                if '*default*' in line:
                    continue
                parts = line.split()
                if len(parts) >= 3:
                    name, origin, length = parts[0], parts[1], parts[2]
                    try:
                        origin_addr = int(origin, 16)
                        length_val = int(length, 16)
                        
                        if name == 'FLASH':
                            self.analysis.total_flash = length_val
                        elif name == 'RAM':
                            self.analysis.total_ram = length_val
                            
                        print(f"  Memory region {name}: {origin} ({length_val} bytes)")
                    except ValueError:
                        continue
    
    def _parse_memory_sections(self, content: str):
        """Parse memory sections from the map file."""
        # Look for section information
        section_patterns = [
            (r'\.text\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', 'code'),
            (r'\.data\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', 'data'),
            (r'\.bss\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', 'bss'),
            (r'\.rodata\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', 'rodata'),
        ]
        
        for pattern, usage_type in section_patterns:
            matches = re.finditer(pattern, content)
            for match in matches:
                address = int(match.group(1), 16)
                size = int(match.group(2), 16)
                if size > 0:  # Only include non-empty sections
                    section_name = f"{usage_type}_{address:08x}"
                    self.analysis.sections[section_name] = MemorySection(
                        name=section_name,
                        address=address,
                        size=size,
                        usage_type=usage_type
                    )
    
    def _parse_function_symbols(self, content: str):
        """Extract function symbols and their sizes."""
        # Look for CBOR-related functions
        cbor_pattern = r'^\s*0x([0-9a-f]+)\s+(.*cbor.*?)(?:\s+0x([0-9a-f]+))?$'
        
        for line in content.split('\n'):
            # Look for function definitions
            if '.text.' in line and 'cbor' in line:
                # Parse function sections like .text.cbor_parse
                func_match = re.search(r'\.text\.(\w*cbor\w*)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)', line)
                if func_match:
                    func_name = func_match.group(1)
                    address = int(func_match.group(2), 16)
                    size = int(func_match.group(3), 16)
                    
                    self.cbor_functions.add(func_name)
                    self.analysis.functions[func_name] = FunctionInfo(
                        name=func_name,
                        address=address,
                        size=size,
                        section='text'
                    )
    
    def _calculate_memory_usage(self):
        """Calculate total memory usage."""
        for section in self.analysis.sections.values():
            if section.usage_type in ['code', 'rodata']:
                self.analysis.used_flash += section.size
            elif section.usage_type in ['data', 'bss']:
                self.analysis.used_ram += section.size
    
    def analyze_stack_usage(self, build_dir: str):
        """Analyze stack usage using GCC stack usage information."""
        print(f"🔍 Analyzing stack usage from build directory: {build_dir}")
        
        # Look for .su files (stack usage files)
        su_files = list(Path(build_dir).glob('**/*.su'))
        
        for su_file in su_files:
            self._parse_stack_usage_file(su_file)
    
    def _parse_stack_usage_file(self, su_file: Path):
        """Parse GCC .su (stack usage) file."""
        try:
            with open(su_file, 'r') as f:
                for line in f:
                    parts = line.strip().split('\t')
                    if len(parts) >= 3:
                        func_name = parts[0].split(':')[-1]
                        stack_usage = int(parts[1])
                        usage_type = parts[2]
                        
                        if func_name in self.analysis.functions:
                            self.analysis.functions[func_name].stack_usage = stack_usage
                        elif 'cbor' in func_name.lower():
                            # Add new CBOR function found in stack usage
                            self.analysis.functions[func_name] = FunctionInfo(
                                name=func_name,
                                address=0,
                                size=0,
                                section='unknown',
                                stack_usage=stack_usage
                            )
        except Exception as e:
            print(f"Warning: Could not parse {su_file}: {e}")
    
    def generate_call_graph(self, elf_file: str):
        """Generate call graph using objdump."""
        print(f"📈 Generating call graph from {elf_file}")
        
        try:
            # Use objdump to disassemble and find function calls
            result = subprocess.run([
                'arm-none-eabi-objdump', '-d', elf_file
            ], capture_output=True, text=True, check=True)
            
            self._parse_objdump_output(result.stdout)
            
        except subprocess.CalledProcessError as e:
            print(f"Warning: Could not run objdump: {e}")
        except FileNotFoundError:
            print("Warning: arm-none-eabi-objdump not found")
    
    def _parse_objdump_output(self, objdump_output: str):
        """Parse objdump output to extract function calls."""
        current_function = None
        
        for line in objdump_output.split('\n'):
            # Look for function headers
            func_match = re.match(r'^([0-9a-f]+) <(.+)>:$', line)
            if func_match:
                current_function = func_match.group(2)
                continue
            
            # Look for branch/call instructions
            if current_function and ('bl\t' in line or 'b\t' in line):
                call_match = re.search(r'<(.+)>', line)
                if call_match:
                    called_func = call_match.group(1)
                    
                    # Update call relationships
                    if current_function in self.analysis.functions:
                        self.analysis.functions[current_function].calls.append(called_func)
                    
                    if called_func in self.analysis.functions:
                        self.analysis.functions[called_func].called_by.append(current_function)
    
    def calculate_worst_case_stack(self):
        """Calculate worst-case stack usage through call path analysis."""
        print("🎯 Calculating worst-case stack usage...")
        
        max_stack = 0
        worst_path = []
        
        # Find all entry points (functions not called by others in our analysis)
        entry_points = []
        for func_name, func_info in self.analysis.functions.items():
            if not func_info.called_by or func_name in ['main', 'Reset_Handler']:
                entry_points.append(func_name)
        
        # Calculate stack usage for each entry point
        for entry in entry_points:
            stack_usage, path = self._calculate_stack_path(entry, set())
            if stack_usage > max_stack:
                max_stack = stack_usage
                worst_path = path
        
        self.analysis.worst_case_stack = max_stack
        return max_stack, worst_path
    
    def _calculate_stack_path(self, func_name: str, visited: set) -> Tuple[int, List[str]]:
        """Recursively calculate stack usage for a function path."""
        if func_name in visited:
            return 0, []  # Prevent infinite recursion
        
        if func_name not in self.analysis.functions:
            return 0, []
        
        func_info = self.analysis.functions[func_name]
        visited.add(func_name)
        
        current_stack = func_info.stack_usage or 0
        max_child_stack = 0
        worst_child_path = []
        
        # Check all called functions
        for called_func in func_info.calls:
            child_stack, child_path = self._calculate_stack_path(called_func, visited.copy())
            if child_stack > max_child_stack:
                max_child_stack = child_stack
                worst_child_path = child_path
        
        total_stack = current_stack + max_child_stack
        path = [func_name] + worst_child_path
        
        return total_stack, path
    
    def generate_report(self) -> str:
        """Generate comprehensive memory analysis report."""
        report = []
        
        report.append("# 🔬 COMPREHENSIVE CBOR MEMORY ANALYSIS REPORT")
        report.append("=" * 60)
        report.append("")
        
        # Memory overview
        report.append("## 📊 Memory Overview")
        report.append(f"- **Total Flash**: {self.analysis.total_flash:,} bytes ({self.analysis.total_flash // 1024} KB)")
        report.append(f"- **Total RAM**: {self.analysis.total_ram:,} bytes ({self.analysis.total_ram // 1024} KB)")
        report.append(f"- **Used Flash**: {self.analysis.used_flash:,} bytes ({self.analysis.used_flash / self.analysis.total_flash * 100:.1f}%)")
        report.append(f"- **Used RAM**: {self.analysis.used_ram:,} bytes ({self.analysis.used_ram / self.analysis.total_ram * 100:.1f}%)")
        report.append("")
        
        # Function analysis
        report.append("## 🔧 CBOR Function Analysis")
        report.append("| Function | Size (bytes) | Stack Usage | Calls |")
        report.append("|----------|--------------|-------------|-------|")
        
        for func_name, func_info in sorted(self.analysis.functions.items()):
            stack_usage = func_info.stack_usage or "N/A"
            calls_count = len(func_info.calls)
            report.append(f"| `{func_name}` | {func_info.size} | {stack_usage} | {calls_count} |")
        
        report.append("")
        
        # Stack analysis
        if self.analysis.worst_case_stack > 0:
            report.append("## 📈 Stack Usage Analysis")
            report.append(f"- **Worst-case stack usage**: {self.analysis.worst_case_stack} bytes")
            
            # Stack safety recommendations
            report.append("")
            report.append("### 🛡️ Stack Safety Recommendations")
            
            recommended_stack = self.analysis.worst_case_stack * 2  # 100% safety margin
            report.append(f"- **Recommended minimum stack**: {recommended_stack} bytes")
            report.append(f"- **Safety margin**: 100% ({self.analysis.worst_case_stack} bytes)")
            
            if recommended_stack <= 1024:
                report.append("- ✅ **Safe for microcontrollers** with 1KB+ stack")
            elif recommended_stack <= 2048:
                report.append("- ⚠️ **Requires 2KB+ stack** for embedded safety")
            else:
                report.append("- ❌ **High stack usage** - consider optimization")
        
        # Memory sections
        report.append("")
        report.append("## 🗺️ Memory Section Analysis")
        report.append("| Section | Address | Size | Type |")
        report.append("|---------|---------|------|------|")
        
        for section_name, section in sorted(self.analysis.sections.items()):
            report.append(f"| {section.name} | 0x{section.address:08x} | {section.size} | {section.usage_type} |")
        
        return "\\n".join(report)

def main():
    parser = argparse.ArgumentParser(description='CBOR Memory Analysis Tool')
    parser.add_argument('map_file', help='Path to the ARM linker map file')
    parser.add_argument('--elf-file', help='Path to the ELF file for call graph analysis')
    parser.add_argument('--build-dir', help='Build directory for stack usage files')
    parser.add_argument('--output', '-o', help='Output file for the report')
    
    args = parser.parse_args()
    
    analyzer = CBORMemoryAnalyzer(args.map_file)
    
    print("🚀 Starting comprehensive CBOR memory analysis...")
    
    # Parse map file
    analyzer.parse_map_file()
    
    # Analyze stack usage if build directory provided
    if args.build_dir:
        analyzer.analyze_stack_usage(args.build_dir)
    
    # Generate call graph if ELF file provided
    if args.elf_file:
        analyzer.generate_call_graph(args.elf_file)
    
    # Calculate worst-case stack usage
    worst_stack, worst_path = analyzer.calculate_worst_case_stack()
    
    if worst_stack > 0:
        print(f"📊 Worst-case stack usage: {worst_stack} bytes")
        print(f"📍 Worst path: {' -> '.join(worst_path[:5])}{'...' if len(worst_path) > 5 else ''}")
    
    # Generate report
    report = analyzer.generate_report()
    
    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(report)
        print(f"📝 Report saved to {args.output}")
    else:
        print("\\n" + report)

if __name__ == '__main__':
    main()
