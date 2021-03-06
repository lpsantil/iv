describe("Map", function() {
  describe('constructor', function() {
    it("with no arguments", function() {
      expect(function() {
        var map = new Map();
      }).not.toThrow();
    });

    it("with undefined", function() {
      expect(function() {
        var map = new Map(undefined);
      }).not.toThrow();
    });

    it("with null", function() {
      expect(function() {
        var map = new Map(null);
      }).toThrow();
    });

    it("with initializer", function() {
      var map = new Map([[0, 0], [-0, 1], ['0', 2]]);
      expect(map.has(0)).toBe(true);
      expect(map.has(-0)).toBe(true);
      expect(map.has('0')).toBe(true);
      // Since 0 and -0 are not distinguished (SameValueZero)
      expect(map.get(0)).toBe(1);
      expect(map.get(-0)).toBe(1);
      expect(map.get('0')).toBe(2);
    });

    it("with empty array", function() {
      expect(function() {
        new Map([]);
      }).not.toThrow();
    });

    it("with curious initializer", function() {
      expect(function() {
        new Map([0]);
      }).not.toThrow();
      expect(function() {
        new Map(['st']);
      }).not.toThrow();

      var map = new Map(['st', 0]);
      expect(map.has(undefined)).toBe(true);
      expect(map.has('s')).toBe(true);
      expect(map.get(undefined)).toBe(undefined);
      expect(map.get('s')).toBe('t');
    });
  });
});
