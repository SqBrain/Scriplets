AVLayer.prototype.instance = "Layer";
TextLayer.prototype.instance = "Layer";
ShapeLayer.prototype.instance = "Layer";
CameraLayer.prototype.instance = "Layer";
LightLayer.prototype.instance = "Layer";
Property.prototype.instance = "Property";
PropertyGroup.prototype.instance = "Property";

Property.prototype.containingComp = function () {
    return this.propertyGroup( this.propertyDepth ).containingComp;
}
Property.prototype.containingLayer = function () {
    return this.propertyGroup( this.propertyDepth );
}
PropertyGroup.prototype.containingComp = function () {
    return this.propertyGroup( this.propertyDepth ).containingComp;
}
PropertyGroup.prototype.containingLayer = function () {
    return this.propertyGroup( this.propertyDepth );
}