import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';

class Hole extends LeafRenderObjectWidget {
  const Hole({Key? key}) : super(key: key);

  @override
  HoleBox createRenderObject(BuildContext context) => HoleBox();
}

class HoleBox extends RenderBox {
  HoleBox();
  @override
  bool get sizedByParent => true;

  @override
  bool get alwaysNeedsCompositing => true;

  @override
  bool get isRepaintBoundary => true;

  @override
  void performResize() {
    size = constraints.biggest;
  }

  @override
  bool hitTestSelf(Offset position) {
    return true;
  }

  @override
  void paint(PaintingContext context, Offset offset) {
    context.addLayer(HoleLayer(rect: offset & size));
  }
}

class HoleLayer extends Layer {
  final Rect rect;

  HoleLayer({
    required this.rect,
  });

  @override
  void addToScene(SceneBuilder builder, [Offset layerOffset = Offset.zero]) {
    builder.addPicture(layerOffset, createHolePicture(rect));
  }

  Picture createHolePicture(Rect holeRect) {
    PictureRecorder recorder = PictureRecorder();
    Canvas canvas = Canvas(recorder);
    Paint paint = Paint();
    paint.color = Colors.transparent;
    paint.blendMode = BlendMode.src;
    canvas.drawRect(
        Rect.fromLTWH(holeRect.topLeft.dx, holeRect.topLeft.dy, holeRect.width,
            holeRect.height),
        paint);
    return recorder.endRecording();
  }
}
