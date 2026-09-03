import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.app.util.parser.FunctionSignatureParser;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.FunctionDefinitionDataType;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolTable;
import ghidra.util.task.ConsoleTaskMonitor;
import ghidra.util.task.TaskMonitor;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;

/** Headless HTTP transport for the Ghidra APIs used by the MCP bridge. */
public class StartGhidraMCP extends GhidraScript {
    private static final int PORT = 8080;
    private Program program;
    private HttpServer server;

    @Override
    public void run() throws Exception {
        if (currentProgram == null) {
            throw new IllegalStateException("No Ghidra program is open");
        }

        program = currentProgram;
        println("Program: " + program.getName());
        println("Language: " + program.getLanguage().getLanguageID());

        server = HttpServer.create(new InetSocketAddress(PORT), 0);
        server.createContext("/", this::handleRequest);
        server.setExecutor(null);
        server.start();
        println("Headless GhidraMCP HTTP server started on port " + PORT);

        new CountDownLatch(1).await();
    }

    private void handleRequest(HttpExchange exchange) throws IOException {
        String path = exchange.getRequestURI().getPath();
        Map<String, String> query = parseQuery(exchange.getRequestURI().getRawQuery());
        String response;

        try {
            if (path.equals("/methods")) {
                response = listMethods(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/classes")) {
                response = listClasses(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/segments")) {
                response = listSegments(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/imports")) {
                response = listImports(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/exports")) {
                response = listExports(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/namespaces")) {
                response = listNamespaces(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/data")) {
                response = listData(integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/strings")) {
                response = listStrings(integer(query, "offset", 0), integer(query, "limit", 2000), query.get("filter"));
            } else if (path.equals("/searchFunctions")) {
                response = searchFunctions(query.get("query"), integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/list_functions")) {
                response = listFunctions();
            } else if (path.equals("/decompile")) {
                response = decompileByName(readBody(exchange));
            } else if (path.equals("/decompile_function")) {
                response = decompileByAddress(query.get("address"));
            } else if (path.equals("/disassemble_function")) {
                response = disassemble(query.get("address"));
            } else if (path.equals("/get_function_by_address")) {
                response = functionInfo(query.get("address"));
            } else if (path.equals("/xrefs_to")) {
                response = xrefsTo(query.get("address"), integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/xrefs_from")) {
                response = xrefsFrom(query.get("address"), integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/function_xrefs")) {
                response = functionXrefs(query.get("name"), integer(query, "offset", 0), integer(query, "limit", 100));
            } else if (path.equals("/renameFunction")) {
                Map<String, String> body = parseQuery(readBody(exchange));
                response = renameFunction(body.get("oldName"), body.get("newName"));
            } else if (path.equals("/rename_function_by_address")) {
                Map<String, String> body = parseQuery(readBody(exchange));
                response = renameFunctionAtAddress(body.get("function_address"), body.get("new_name"));
            } else if (path.equals("/renameData")) {
                Map<String, String> body = parseQuery(readBody(exchange));
                response = renameData(body.get("address"), body.get("newName"));
            } else if (path.equals("/set_decompiler_comment")) {
                Map<String, String> body = parseQuery(readBody(exchange));
                response = setComment(body.get("address"), body.get("comment"), CodeUnit.PRE_COMMENT);
            } else if (path.equals("/set_disassembly_comment")) {
                Map<String, String> body = parseQuery(readBody(exchange));
                response = setComment(body.get("address"), body.get("comment"), CodeUnit.EOL_COMMENT);
            } else if (path.equals("/set_function_prototype")) {
                Map<String, String> body = parseQuery(readBody(exchange));
                response = setPrototype(body.get("function_address"), body.get("prototype"));
            } else if (path.equals("/get_current_address") || path.equals("/get_current_function")) {
                response = "No current location in headless mode";
            } else {
                response = "Unsupported headless endpoint: " + path;
            }
        } catch (Exception e) {
            response = "Error: " + e.getMessage();
        }

        byte[] bytes = response.getBytes(StandardCharsets.UTF_8);
        exchange.getResponseHeaders().set("Content-Type", "text/plain; charset=utf-8");
        exchange.sendResponseHeaders(200, bytes.length);
        try (OutputStream output = exchange.getResponseBody()) {
            output.write(bytes);
        }
    }

    private String listMethods(int offset, int limit) {
        List<String> names = new ArrayList<String>();
        for (Function function : program.getFunctionManager().getFunctions(true)) {
            names.add(function.getName());
        }
        return paginate(names, offset, limit);
    }

    private String listClasses(int offset, int limit) {
        Set<String> names = new HashSet<String>();
        SymbolIterator symbols = program.getSymbolTable().getAllSymbols(true);
        while (symbols.hasNext()) {
            Namespace namespace = symbols.next().getParentNamespace();
            if (namespace != null && !namespace.isGlobal()) {
                names.add(namespace.getName());
            }
        }
        List<String> sorted = new ArrayList<String>(names);
        Collections.sort(sorted);
        return paginate(sorted, offset, limit);
    }

    private String listSegments(int offset, int limit) {
        List<String> lines = new ArrayList<String>();
        for (MemoryBlock block : program.getMemory().getBlocks()) {
            lines.add(block.getName() + ": " + block.getStart() + " - " + block.getEnd());
        }
        return paginate(lines, offset, limit);
    }

    private String listImports(int offset, int limit) {
        List<String> lines = new ArrayList<String>();
        SymbolIterator symbols = program.getSymbolTable().getExternalSymbols();
        while (symbols.hasNext()) {
            Symbol symbol = symbols.next();
            lines.add(symbol.getName() + " -> " + symbol.getAddress());
        }
        return paginate(lines, offset, limit);
    }

    private String listExports(int offset, int limit) {
        List<String> lines = new ArrayList<String>();
        SymbolIterator symbols = program.getSymbolTable().getAllSymbols(true);
        while (symbols.hasNext()) {
            Symbol symbol = symbols.next();
            if (symbol.isExternalEntryPoint()) {
                lines.add(symbol.getName() + " -> " + symbol.getAddress());
            }
        }
        return paginate(lines, offset, limit);
    }

    private String listNamespaces(int offset, int limit) {
        Set<String> names = new HashSet<String>();
        SymbolIterator symbols = program.getSymbolTable().getAllSymbols(true);
        while (symbols.hasNext()) {
            Namespace namespace = symbols.next().getParentNamespace();
            if (namespace != null && !namespace.isGlobal()) {
                names.add(namespace.getName());
            }
        }
        List<String> sorted = new ArrayList<String>(names);
        Collections.sort(sorted);
        return paginate(sorted, offset, limit);
    }

    private String listData(int offset, int limit) {
        List<String> lines = new ArrayList<String>();
        Listing listing = program.getListing();
        for (MemoryBlock block : program.getMemory().getBlocks()) {
            DataIterator data = listing.getDefinedData(block.getStart(), true);
            while (data.hasNext()) {
                Data item = data.next();
                if (block.contains(item.getAddress())) {
                    String label = item.getLabel() == null ? "(unnamed)" : item.getLabel();
                    lines.add(item.getAddress() + ": " + label + " = " + item.getDefaultValueRepresentation());
                }
            }
        }
        return paginate(lines, offset, limit);
    }

    private String listStrings(int offset, int limit, String filter) {
        List<String> lines = new ArrayList<String>();
        Listing listing = program.getListing();
        for (MemoryBlock block : program.getMemory().getBlocks()) {
            DataIterator data = listing.getDefinedData(block.getStart(), true);
            while (data.hasNext()) {
                Data item = data.next();
                Object value = item.getValue();
                if (!(value instanceof String)) {
                    continue;
                }
                String text = (String) value;
                if (filter == null || text.toLowerCase().contains(filter.toLowerCase())) {
                    lines.add(item.getAddress() + ": " + text);
                }
            }
        }
        return paginate(lines, offset, limit);
    }

    private String searchFunctions(String query, int offset, int limit) {
        if (query == null || query.length() == 0) {
            return "Search term is required";
        }
        List<String> lines = new ArrayList<String>();
        for (Function function : program.getFunctionManager().getFunctions(true)) {
            if (function.getName().toLowerCase().contains(query.toLowerCase())) {
                lines.add(function.getName() + " @ " + function.getEntryPoint());
            }
        }
        Collections.sort(lines);
        return paginate(lines, offset, limit);
    }

    private String listFunctions() {
        StringBuilder result = new StringBuilder();
        for (Function function : program.getFunctionManager().getFunctions(true)) {
            result.append(function.getName()).append(" at ").append(function.getEntryPoint()).append('\n');
        }
        return result.toString();
    }

    private String decompileByName(String name) {
        for (Function function : program.getFunctionManager().getFunctions(true)) {
            if (function.getName().equals(name)) {
                return decompile(function);
            }
        }
        return "Function not found";
    }

    private String decompileByAddress(String text) {
        Function function = functionAt(text);
        return function == null ? "No function found at or containing address " + text : decompile(function);
    }

    private String decompile(Function function) {
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(program);
        DecompileResults result = decompiler.decompileFunction(function, 60, new ConsoleTaskMonitor());
        return result != null && result.decompileCompleted()
            ? result.getDecompiledFunction().getC()
            : "Decompilation failed: " + (result == null ? "no result" : result.getErrorMessage());
    }

    private String disassemble(String text) {
        Function function = functionAt(text);
        if (function == null) {
            return "No function found at or containing address " + text;
        }
        StringBuilder result = new StringBuilder();
        Address end = function.getBody().getMaxAddress();
        InstructionIterator instructions = program.getListing().getInstructions(function.getEntryPoint(), true);
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            if (instruction.getAddress().compareTo(end) > 0) {
                break;
            }
            result.append(instruction.getAddress()).append(": ").append(instruction).append('\n');
        }
        return result.toString();
    }

    private String functionInfo(String text) {
        Function function = functionAt(text);
        return function == null ? "No function found at or containing address " + text
            : "Function: " + function.getName() + " at " + function.getEntryPoint()
            + "\nSignature: " + function.getSignature()
            + "\nEntry: " + function.getEntryPoint()
            + "\nBody: " + function.getBody().getMinAddress() + " - " + function.getBody().getMaxAddress();
    }

    private String xrefsTo(String text, int offset, int limit) {
        Address address = address(text);
        List<String> lines = new ArrayList<String>();
        ReferenceIterator references = program.getReferenceManager().getReferencesTo(address);
        while (references.hasNext()) {
            Reference reference = references.next();
            lines.add(reference.getFromAddress() + " -> " + reference.getToAddress() + " (" + reference.getReferenceType() + ")");
        }
        return paginate(lines, offset, limit);
    }

    private String xrefsFrom(String text, int offset, int limit) {
        Address address = address(text);
        List<String> lines = new ArrayList<String>();
        Reference[] references = program.getReferenceManager().getReferencesFrom(address);
        for (Reference reference : references) {
            lines.add(reference.getFromAddress() + " -> " + reference.getToAddress() + " (" + reference.getReferenceType() + ")");
        }
        return paginate(lines, offset, limit);
    }

    private String functionXrefs(String name, int offset, int limit) {
        for (Function function : program.getFunctionManager().getFunctions(true)) {
            if (function.getName().equals(name)) {
                return xrefsTo(function.getEntryPoint().toString(), offset, limit);
            }
        }
        return "Function not found";
    }

    private String renameFunction(String oldName, String newName) {
        for (Function function : program.getFunctionManager().getFunctions(true)) {
            if (function.getName().equals(oldName)) {
                return rename(function, newName);
            }
        }
        return "Rename failed";
    }

    private String renameFunctionAtAddress(String text, String newName) {
        Function function = functionAt(text);
        return function == null ? "Failed to rename function" : rename(function, newName);
    }

    private String rename(Function function, String newName) {
        int transaction = program.startTransaction("Headless GhidraMCP rename function");
        boolean success = false;
        try {
            function.setName(newName, SourceType.USER_DEFINED);
            success = true;
            return "Renamed successfully";
        } catch (Exception e) {
            return "Rename failed: " + e.getMessage();
        } finally {
            program.endTransaction(transaction, success);
        }
    }

    private String renameData(String text, String newName) {
        int transaction = program.startTransaction("Headless GhidraMCP rename data");
        boolean success = false;
        try {
            Address address = address(text);
            Symbol symbol = program.getSymbolTable().getPrimarySymbol(address);
            if (symbol == null) {
                program.getSymbolTable().createLabel(address, newName, SourceType.USER_DEFINED);
            } else {
                symbol.setName(newName, SourceType.USER_DEFINED);
            }
            success = true;
            return "Rename data attempted";
        } catch (Exception e) {
            return "Rename data failed: " + e.getMessage();
        } finally {
            program.endTransaction(transaction, success);
        }
    }

    private String setComment(String text, String comment, int type) {
        int transaction = program.startTransaction("Headless GhidraMCP comment");
        boolean success = false;
        try {
            program.getListing().setComment(address(text), type, comment);
            success = true;
            return "Comment set successfully";
        } finally {
            program.endTransaction(transaction, success);
        }
    }

    private String setPrototype(String text, String prototype) {
        Address address = address(text);
        FunctionSignatureParser parser = new FunctionSignatureParser(program.getDataTypeManager(), null);
        try {
            FunctionDefinitionDataType signature = parser.parse(null, prototype);
            if (signature == null) {
                return "Failed to parse function prototype";
            }
            ApplyFunctionSignatureCmd command = new ApplyFunctionSignatureCmd(address, signature, SourceType.USER_DEFINED);
            return command.applyTo(program, new ConsoleTaskMonitor()) ? "Function prototype set successfully" : "Failed to set function prototype";
        } catch (Exception e) {
            return "Failed to set function prototype: " + e.getMessage();
        }
    }

    private Function functionAt(String text) {
        Address address = address(text);
        Function function = program.getFunctionManager().getFunctionAt(address);
        return function == null ? program.getFunctionManager().getFunctionContaining(address) : function;
    }

    private Address address(String text) {
        if (text == null || text.length() == 0) {
            throw new IllegalArgumentException("Address is required");
        }
        return program.getAddressFactory().getAddress(text);
    }

    private static String paginate(List<String> values, int offset, int limit) {
        int start = Math.max(0, Math.min(offset, values.size()));
        int end = Math.max(start, Math.min(start + Math.max(0, limit), values.size()));
        StringBuilder result = new StringBuilder();
        for (int i = start; i < end; i++) {
            result.append(values.get(i)).append('\n');
        }
        return result.toString();
    }

    private static int integer(Map<String, String> values, String key, int fallback) {
        try {
            return Integer.parseInt(values.get(key));
        } catch (Exception e) {
            return fallback;
        }
    }

    private static String readBody(HttpExchange exchange) throws IOException {
        return new String(exchange.getRequestBody().readAllBytes(), StandardCharsets.UTF_8);
    }

    private static Map<String, String> parseQuery(String query) {
        Map<String, String> values = new HashMap<String, String>();
        if (query == null || query.length() == 0) {
            return values;
        }
        for (String part : query.split("&")) {
            String[] pair = part.split("=", 2);
            String key = decode(pair[0]);
            String value = pair.length == 2 ? decode(pair[1]) : "";
            values.put(key, value);
        }
        return values;
    }

    private static String decode(String value) {
        try {
            return URLDecoder.decode(value, "UTF-8");
        } catch (Exception e) {
            return value;
        }
    }
}
