import { Activity, Hash, Volume2 } from 'lucide-react';
import { ChessBoard } from '@/components/chessboard/ChessBoard';
import { PageTransition } from '@/components/layout/PageTransition';
import { BoardThemeSelector } from '@/components/settings/BoardThemeSelector';
import { PieceThemeSelector } from '@/components/settings/PieceThemeSelector';
import { ToggleSetting } from '@/components/settings/ToggleSetting';
import { Button } from '@/components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';
import { useSettingsStore } from '@/store/settingsStore';
import { START_FEN } from '@/utils/chess';

export function SettingsPage() {
  const soundEnabled = useSettingsStore((state) => state.soundEnabled);
  const setSoundEnabled = useSettingsStore((state) => state.setSoundEnabled);
  const animationsEnabled = useSettingsStore((state) => state.animationsEnabled);
  const setAnimationsEnabled = useSettingsStore((state) => state.setAnimationsEnabled);
  const coordinatesVisible = useSettingsStore((state) => state.coordinatesVisible);
  const setCoordinatesVisible = useSettingsStore((state) => state.setCoordinatesVisible);
  const orientation = useSettingsStore((state) => state.boardOrientation);
  const resetSettings = useSettingsStore((state) => state.resetSettings);

  return (
    <PageTransition className="mx-auto max-w-6xl">
      <div className="mb-6 flex items-center justify-between gap-4">
        <h1 className="text-2xl font-bold tracking-tight">Settings</h1>
        <Button variant="outline" size="sm" onClick={resetSettings}>
          Restore defaults
        </Button>
      </div>

      <div className="grid items-start gap-6 lg:grid-cols-[minmax(0,1fr)_22rem]">
        <div className="space-y-6">
          <Card>
            <CardHeader>
              <CardTitle>Board theme</CardTitle>
              <CardDescription>Square colors and highlight styles.</CardDescription>
            </CardHeader>
            <CardContent>
              <BoardThemeSelector />
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle>Piece theme</CardTitle>
              <CardDescription>Switch piece sets without touching board logic.</CardDescription>
            </CardHeader>
            <CardContent>
              <PieceThemeSelector />
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle>Preferences</CardTitle>
            </CardHeader>
            <CardContent className="space-y-4">
              <ToggleSetting
                id="setting-sound"
                label="Sound"
                description="Play sounds for moves, captures, castling, promotion and checks."
                icon={<Volume2 className="size-4" aria-hidden />}
                checked={soundEnabled}
                onCheckedChange={setSoundEnabled}
              />
              <Separator />
              <ToggleSetting
                id="setting-animations"
                label="Animations"
                description="Animate pieces, panels and the evaluation bar."
                icon={<Activity className="size-4" aria-hidden />}
                checked={animationsEnabled}
                onCheckedChange={setAnimationsEnabled}
              />
              <Separator />
              <ToggleSetting
                id="setting-coordinates"
                label="Coordinates"
                description="Show file and rank labels on the board."
                icon={<Hash className="size-4" aria-hidden />}
                checked={coordinatesVisible}
                onCheckedChange={setCoordinatesVisible}
              />
            </CardContent>
          </Card>
        </div>

        <aside className="lg:sticky lg:top-20">
          <Card>
            <CardHeader>
              <CardTitle>Preview</CardTitle>
            </CardHeader>
            <CardContent>
              <ChessBoard
                boardId="settings-preview"
                fen={START_FEN}
                orientation={orientation}
                showCoordinates={coordinatesVisible}
                interactive={false}
              />
            </CardContent>
          </Card>
        </aside>
      </div>
    </PageTransition>
  );
}
